#include <linux/bio.h>
#include <linux/buffer_head.h>
#include <linux/types.h>
#include <linux/blk-mq.h>	
#include <linux/module.h>
#include <linux/kdev_t.h>
#include <linux/sched.h>
#include <linux/genhd.h>		
#include <linux/fs.h>			
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/errno.h>
#include <linux/blkdev.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/version.h> 	

static int dof_major = 0;

#define DOF_MINORS 2
#define KERNEL_SECTOR_SIZE 512
#define MBR_SIZE KERNEL_SECTOR_SIZE
#define MBR_DISK_SIGNATURE_OFFSET 440
#define MBR_DISK_SIGNATURE_SIZE 4
#define PARTITION_TABLE_OFFSET 446
#define PARTITION_ENTRY_SIZE 16 
#define PARTITION_TABLE_SIZE 64 
#define MBR_SIGNATURE_OFFSET 510
#define MBR_SIGNATURE_SIZE 2
#define MBR_SIGNATURE 0xAA55

static int disk_sect_size = 512;
static int nsectors = 1024;	

typedef struct
{
	unsigned char boot_type; 
	unsigned char start_head;
	unsigned char start_sec:6;
	unsigned char start_cyl_hi:2;
	unsigned char start_cyl;
	unsigned char part_type;
	unsigned char end_head;
	unsigned char end_sec:6;
	unsigned char end_cyl_hi:2;
	unsigned char end_cyl;
	unsigned int abs_start_sec;
	unsigned int sec_in_part;
} PartEntry;

typedef PartEntry PartTable[4];

static PartTable def_part_table =
{
	{
		boot_type: 0x00,
		start_head: 0x00,
		start_sec: 0x2,
		start_cyl: 0x00,
		part_type: 0x83,
		end_head: 0x00,
		end_sec: 0x20,
		end_cyl: 0x09,
		abs_start_sec: 0x00000001,
		sec_in_part: 0x0000013F
	},
	{
		boot_type: 0x00,
		start_head: 0x00,
		start_sec: 0x1,
		start_cyl: 0x14,
		part_type: 0x83,
		end_head: 0x00,
		end_sec: 0x20,
		end_cyl: 0x1F,
		abs_start_sec: 0x00000280,
		sec_in_part: 0x00000180
	},
	{	
	},
	{
	}
};

struct dof_dev {
	/* Size is the size of the device (in sectors) */
        int size;                      
        u8 *data;                       
    /* For exclusive access to our request queue */
	    spinlock_t lock;               
	struct blk_mq_tag_set tag_set; //
	/* Our request queue */
        struct request_queue *queue;   
	/* This is kernel's representation of an individual disk device */
        struct gendisk *gd_dof;            
}dof_device;

static void sbull_transfer(struct dof_dev *dev, unsigned long sector, unsigned long nsect, char *buffer, int write)
{	unsigned long offset = sector*KERNEL_SECTOR_SIZE;
	unsigned long nbytes = nsect*KERNEL_SECTOR_SIZE;
	if ((offset + nbytes) > dev->size) {
		printk (KERN_NOTICE "Beyond-end write ...exceeded (%ld %ld)\n", offset, nbytes);
		return;
	}
	if (write)
		memcpy(dev->data + offset, buffer, nbytes);
	else
		memcpy(buffer, dev->data + offset, nbytes);
}


#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 9, 0))
static inline struct request_queue *
blk_generic_alloc_queue(make_request_fn make_request, int node_id)
#else
static inline struct request_queue *
blk_generic_alloc_queue(int node_id)
#endif
{
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 7, 0))
	struct request_queue *q = blk_alloc_queue(GFP_KERNEL);
	if (q != NULL)
		blk_queue_make_request(q, make_request);

	return (q);
#elif (LINUX_VERSION_CODE < KERNEL_VERSION(5, 9, 0))
	return (blk_alloc_queue(make_request, node_id));
#else
	return (blk_alloc_queue(node_id));
#endif
}

static int sbull_open(struct block_device *bdev, fmode_t mode)	{
	// unsigned unit = iminor(bdev->bd_inode);
	printk(KERN_INFO "dof : open \n");
	return 0;
}

static void sbull_release(struct gendisk *disk, fmode_t mode){		
	printk(KERN_INFO "dof : closed \n");
}

static void copy_mbr(u8 *disk) {	
	memset(disk, 0x0, MBR_SIZE);
	*(unsigned long *)(disk + MBR_DISK_SIGNATURE_OFFSET) = 0x36E5756D;
	memcpy(disk + PARTITION_TABLE_OFFSET, &def_part_table, PARTITION_TABLE_SIZE);
	*(unsigned short *)(disk + MBR_SIGNATURE_OFFSET) = MBR_SIGNATURE;
}



static blk_status_t sbull_request(struct blk_mq_hw_ctx *hctx, const struct blk_mq_queue_data* bd)   /* For blk-mq */
{
	struct request *req = bd->rq;
	struct dof_dev *dev = req->rq_disk->private_data;
        struct bio_vec bvec;
        struct req_iterator iter;
        sector_t pos_sector = blk_rq_pos(req);
	void	*buffer;
	blk_status_t  ret;

	blk_mq_start_request (req);

	if (blk_rq_is_passthrough(req)) {
		printk (KERN_NOTICE "Skip non-filesystem request\n");
                ret = BLK_STS_IOERR;  
			goto done;
	}
	rq_for_each_segment(bvec, req, iter)
	{
		size_t num_sector = blk_rq_cur_sectors(req);
		printk (KERN_NOTICE "dir %d sec %lld, nr %ld\n",
                        rq_data_dir(req),
                        pos_sector, num_sector);
		buffer = page_address(bvec.bv_page) + bvec.bv_offset;
		sbull_transfer(dev, pos_sector, num_sector,
				buffer, rq_data_dir(req) == WRITE);
		pos_sector += num_sector;
	}
	ret = BLK_STS_OK;
done:
	blk_mq_end_request (req, ret);
	return ret;
}

static int sbull_xfer_bio(struct dof_dev *dev, struct bio *bio)
{   
	// int i;
	struct bio_vec bvec;
	struct bvec_iter iteration;
	int dir = bio_data_dir(bio);
	
	bio_for_each_segment(bvec,bio,iteration){
	
		sector_t sector = iteration.bi_sector;
		char *buffer = kmap_atomic(bvec.bv_page);
		unsigned long offset = bvec.bv_offset;
		size_t len = bvec.bv_len;
	        
	        /* Process mapped buffer */
	        sbull_transfer(dev, sector, len, buffer+offset, dir);
	        kunmap_atomic(buffer);
	}
	return 0;
}


static int sbull_xfer_request(struct dof_dev *dev, struct request *req) {
	struct bio *bio;
	int nsect = 0;
		__rq_for_each_bio(bio, req) {
		sbull_xfer_bio(dev, bio);
		nsect += bio->bi_iter.bi_size/KERNEL_SECTOR_SIZE;
		}
	return nsect;
}

static struct block_device_operations fops =
{
	.owner = THIS_MODULE,
	.open = sbull_open,
	.release = sbull_release,
};

static struct blk_mq_ops mq_ops_simple = 
{
    .queue_rq = sbull_request,
};

static int __init sbull_init(void)
{
	dof_major = register_blkdev(dof_major, "dof");
	if (dof_major <= 0) {
		printk(KERN_INFO "dof: major number not obtained\n");
		return -EBUSY;
	}

    struct dof_dev* dev = &dof_device;
	
	/* Set Device Size */
	dof_device.size = nsectors*disk_sect_size;
	/* Initialization */
	dof_device.data = vmalloc(dof_device.size);

	copy_mbr(dof_device.data);
	/* Get a request queue (here queue is created) */
	spin_lock_init(&dof_device.lock);		
	dof_device.queue = blk_mq_init_sq_queue(&dof_device.tag_set, &mq_ops_simple, 128, BLK_MQ_F_SHOULD_MERGE);
	blk_queue_logical_block_size(dof_device.queue, disk_sect_size);
	(dof_device.queue)->queuedata = dev;

	/*
	 * Add the gendisk structure
	 * By using this memory allocation is involved, 
	 * the minor number we need to pass bcz the device 
	 * will support this much partitions 
	 */
	dof_device.gd_dof = alloc_disk(DOF_MINORS);
	/* Setting the major number */
	dof_device.gd_dof->major = dof_major;
	/* Setting the first mior number */
	dof_device.gd_dof->first_minor = 0;
	dof_device.gd_dof->minors = DOF_MINORS;
	/* Initializing the device operations */
	dof_device.gd_dof->fops = &fops;
	dof_device.gd_dof->queue = dev->queue;
	/* Driver-specific own internal data */
	dof_device.gd_dof->private_data = dev;

	sprintf(dof_device.gd_dof->disk_name,"dof");
	/* Setting the capacity of the device in its gendisk structure */
	set_capacity(dof_device.gd_dof, nsectors*(disk_sect_size/KERNEL_SECTOR_SIZE));
	/* Adding the disk to the system */
	add_disk(dof_device.gd_dof);
	/* Now the disk is "live" */
	// printk(KERN_INFO "dof: DOF Block driver initialised (%d sectors; %d bytes)\n", dof_dev.size, dof_dev.size * DOF_SECTOR_SIZE);	    
	return 0;
}

static void sbull_exit(void)
{
	del_gendisk(dof_device.gd_dof);
	unregister_blkdev(dof_major, "dof");
	put_disk(dof_device.gd_dof);	
	blk_cleanup_queue(dof_device.queue);
	vfree(dof_device.data);
	spin_unlock(&dof_device.lock);	
	printk(KERN_ALERT "dof is unregistered");
}
	
module_init(sbull_init);
module_exit(sbull_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("NAREN <h20150547@pilani.bits-pilani.ac.in");
MODULE_DESCRIPTION("EEE G547 Assignment 2 Block Device Drivers");
