/*
 *  imu_char.c - Kernel Module for 10 DoF Intertial Module Sensor
 * MPU9255 (9 AXES) and BMP280 (1 AXIS)
 */

#include <linux/kernel.h>       /* We're doing kernel work */
#include <linux/module.h>       /* Specifically, a module */
#include <linux/fs.h>
#include <asm/uaccess.h>        /* for get_user and put_user */
#include <linux/time.h>
#include <linux/ktime.h>
#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/random.h>

#include "imudev.h" /*  header file with the ioctl definitions. */
#define SUCCESS 0
#define DEVICE_NAME "imu_char"
#define BUF_LEN 80
#define CALIBRATE_MPU9255 "dummy_calibration_code_9255"
#define CALIBRATE_BMP280 "dummy_calibration_code_280"

static dev_t first; // variable for device number
static struct cdev c_dev; // variable for the character device structure
static struct class *cls; // variable for device class

static int Device_Open = 0;
static char Message[BUF_LEN]; // The message the device will give when asked
/*
 * How far did the process reading the message get?
 * Useful if the message is larger than the size of the
 * buffer we get to fill in device_read.
 */
static char *Message_Ptr;

static int device_open(struct inode *inode, struct file *file)
{
#ifdef DEBUG
        printk(KERN_INFO "device_open(%p)\n", file);
#endif
	
    printk(KERN_INFO "imu_char : open()\n");
    /*
     * We don't want to talk to two processes at the same time
     */
    if (Device_Open)
        return -EBUSY;

    Device_Open++;
    /*
     * Initialize the message
     */
    Message_Ptr = Message;
    try_module_get(THIS_MODULE);
    return SUCCESS;
}

static int device_release(struct inode *inode, struct file *file)
{
#ifdef DEBUG
    printk(KERN_INFO "device_release(%p,%p)\n", inode, file);
#endif
    printk(KERN_INFO "imu_char : release()\n");
    /*
     * We're now ready for our next caller
     */
    Device_Open--;

    module_put(THIS_MODULE);
    return SUCCESS;
}

/*
 * This function is called whenever a process which has already opened the
 * device file attempts to read from it.
 */
static ssize_t device_read(struct file *file,   /* see include/linux/fs.h   */
                           char __user * buffer,        /* buffer to be
                                                         * filled with data */
                           size_t length,       /* length of the buffer     */
                           loff_t * offset)
{
    /*
     * Number of bytes actually written to the buffer
     */
    int bytes_read = 0;

#ifdef DEBUG
    printk(KERN_INFO "device_read(%p,%p,%d)\n", file, buffer, length);
#endif
    printk(KERN_INFO "imu_char : read()\n");
    unsigned long current_time = ktime_get_ns();
    printk(KERN_INFO "imu_char : virtual_sensor : Current time (used for Random Number Generation) = %lu", current_time);
    unsigned long n = __copy_to_user(Message_Ptr, current_time, 4); // Copy data to user space

#ifdef DEBUG
    printk(KERN_INFO "Read %d bytes, %d left\n", bytes_read, length);
#endif

    /*
     * Read functions are supposed to return the number
     * of bytes actually inserted into the buffer
     */
    return bytes_read;
}

/*
 * This function is called when somebody tries to
 * write into our device file.
 */
static ssize_t
device_write(struct file *file,
             const char __user * buffer, size_t length, loff_t * offset)
{
    int i;

#ifdef DEBUG
    printk(KERN_INFO "device_write(%p,%s,%d)", file, buffer, length);
#endif
    printk(KERN_INFO "imu_char : write()\n");
    for (i = 0; i < length && i < BUF_LEN; i++)
        get_user(Message[i], buffer + i);

    Message_Ptr = Message;

    /*
     * Again, return the number of input characters used
     */
    return i;
}


/*
 * This function is called whenever a process tries to do an ioctl on our
 * device file. We get two extra parameters (additional to the inode and file
 * structures, which all device functions get): the number of the ioctl called
 * and the parameter given to the ioctl function.
 *
 * If the ioctl is write or read/write (meaning output is returned to the
 * calling process), the ioctl call returns the output of this function.
 *
 */

long device_ioctl(struct file *file,             /* ditto */
                  unsigned int ioctl_num,        /* number and param for ioctl */
                  unsigned long ioctl_param)
{
    int i;
    char *temp;
    char ch;
    long reading;

    switch (ioctl_num) {
    case IOCTL_SET_MSG:
        /*
         * Receive a pointer to a message (in user space) and set that
         * to be the device's message.  Get the parameter given to
         * ioctl by the process.
         */
        temp = (char *)ioctl_param;

         /*
          * Find the length of the message
          */
         get_user(ch, temp);
         for (i = 0; ch && i < BUF_LEN; i++, temp++)
             get_user(ch, temp);

         device_write(file, (char *)ioctl_param, i, 0);
         break;

    case IOCTL_GET_MSG:
        /*
         * Give the current message to the calling process -
         * the parameter we got is a pointer, fill it.
         */
        i = device_read(file, (char *)ioctl_param, 99, 0);

        /*
         * Put a zero at the end of the buffer, so it will be
         * properly terminated
         */
        put_user('\0', (char *)ioctl_param + i);
        break;

    case IOCTL_GET_NTH_BYTE:
        /*
         * This ioctl is both input (ioctl_param) and
         * output (the return value of this function)
         */
	// ts = current_kernel_time();
        // return (ts.tv_nsec);
        break;

case IOCTL_CALIBRATE_BMP280:
        printk(KERN_INFO "imu_char : IOCTL_CALIBRATE_BMP280 initiated\n");
        device_write(file, (char *)IOCTL_CALIBRATE_BMP280, i, 0); // Write the required code to the device file which will recalibrate it
        printk(KERN_INFO "imu_char : IOCTL_CALIBRATE_BMP280 device write completed \n");
        break;

case IOCTL_CALIBRATE_MPU9255:
        printk(KERN_INFO "imu_char : IOCTL_CALIBRATE_MPU9255 initiated\n");
        device_write(file, (char *)CALIBRATE_MPU9255, i, 0); // Write the required code to the device file which will recalibrate it
        printk(KERN_INFO "imu_char : IOCTL_CALIBRATE_MPU9255 device write completed \n");
        break;


case IOCTL_GYRO_RR:
        printk(KERN_INFO "imu_char : IOCTL_GYRO_RR initiated\n");
        device_write(file, (char *)ioctl_param, i, 0); // Write the corresponding request to the device file which will query the device for the reading
        printk(KERN_INFO "imu_char : IOCTL_GYRO_RR device write completed \n");
        break;

case IOCTL_ACCEL_RR:
        printk(KERN_INFO "imu_char : IOCTL_ACCEL_RR initiated\n");
        device_write(file, (char *)ioctl_param, i, 0); // Write the corresponding request to the device file which will query the device for the reading
        printk(KERN_INFO "imu_char : IOCTL_ACCEL_RR device write completed \n");
        break;

case IOCTL_COMP_RR:
        printk(KERN_INFO "imu_char : IOCTL_COMP_RR initiated\n");
        device_write(file, (char *)ioctl_param, i, 0); // Write the corresponding request to the device file which will query the device for the reading
        printk(KERN_INFO "imu_char : IOCTL_COMP_RR device write completed \n");
        break;

case IOCTL_PRESSURE_RR:
        printk(KERN_INFO "imu_char : IOCTL_PRESSURE_RR initiated\n");
        device_write(file, (char *)ioctl_param, i, 0); // Write the corresponding request to the device file which will query the device for the reading
        printk(KERN_INFO "imu_char : IOCTL_PRESSURE_RR device write completed \n");
        break;
       
case IOCTL_GET_GYRO:
        printk(KERN_INFO "imu_char : IOCTL_GET_GYRO initiated\n");
        device_write(file, (char *)ioctl_param, i, 0); // Write the corresponding request to the device file which will query the device for the reading
        printk(KERN_INFO "imu_char : IOCTL_GET_GYRO device write completed \n");
        i = device_read(file, (char *)ioctl_param, 4, 0);
        printk(KERN_INFO "imu_char : IOCTL_GET_GYRO device read completed \n");
        /*
         * Put a zero at the end of the buffer, so it will be
         * properly terminated
         */
        put_user('\0', (char *)ioctl_param + i);
        break;    
    
case IOCTL_GET_ACCEL:
        printk(KERN_INFO "imu_char : IOCTL_GET_ACCEL initiated\n");
        device_write(file, (char *)ioctl_param, i, 0); // Write the corresponding request to the device file which will query the device for the reading
        printk(KERN_INFO "imu_char : IOCTL_GET_ACCEL device write completed \n");
        i = device_read(file, (char *)ioctl_param, 4, 0);
        printk(KERN_INFO "imu_char : IOCTL_GET_ACCEL device read completed \n");
        /*
         * Put a zero at the end of the buffer, so it will be
         * properly terminated
         */
        put_user('\0', (char *)ioctl_param + i);
        break; 

case IOCTL_GET_COMP:
        printk(KERN_INFO "imu_char : IOCTL_GET_COMP initiated\n");
        device_write(file, (char *)ioctl_param, i, 0); // Write the corresponding request to the device file which will query the device for the reading
        printk(KERN_INFO "imu_char : IOCTL_GET_COMP device write completed \n");
        i = device_read(file, (char *)ioctl_param, 4, 0);
        printk(KERN_INFO "imu_char : IOCTL_GET_COMP device read completed \n");
        /*
         * Put a zero at the end of the buffer, so it will be
         * properly terminated
         */
        put_user('\0', (char *)ioctl_param + i);
        break; 
    
case IOCTL_GET_PRESSURE:
        printk(KERN_INFO "imu_char : IOCTL_GET_PRESSURE initiated\n");
        device_write(file, (char *)ioctl_param, i, 0); // Write the corresponding request to the device file which will query the device for the reading
        printk(KERN_INFO "imu_char : IOCTL_GET_PRESSURE device write completed \n");
        i = device_read(file, (char *)ioctl_param, 4, 0);
        printk(KERN_INFO "imu_char : IOCTL_GET_PRESSURE device read completed \n");
        /*
         * Put a zero at the end of the buffer, so it will be
         * properly terminated
         */
        put_user('\0', (char *)ioctl_param + i);
        break; 

    }

    return SUCCESS;
}

/* Module Declarations */

/*
 * This structure will hold the functions to be called
 * when a process does something to the device we
 * created. Since a pointer to this structure is kept in
 * the devices table, it can't be local to
 * init_module. NULL is for unimplemented functions.
 */
struct file_operations Fops = {
	.owner = THIS_MODULE,
        .read = device_read,
        .write = device_write,
        .unlocked_ioctl = device_ioctl,
        .open = device_open,
        .release = device_release,      /* a.k.a. close */
};

/*
 * Initialize the module - Register the character device
 */
int init_module()
{
    first = MKDEV(MAJOR_NUM,MINOR_NUM);
    
    printk(KERN_INFO "Driver Initiated\n\n");

    // STEP 1: reserve <major,mminor>
    if (register_chrdev_region(first, 1, "imu_char")<0)
    {
        // return -1;
    }
    
    // STEP 2: dynamically create device node in \dev directory
    if ((cls = class_create(THIS_MODULE, "imu_char")) == NULL)
	{
	unregister_chrdev_region(first, 1);
	// return -1;
	}
	
	// get dev_t
	if (device_create(cls, NULL, first, NULL, "imu_char") == NULL)
	{
	class_destroy(cls);
	unregister_chrdev_region(first, 1);
	// return -1;
	}
	
    // STEP 3 : Link fops and cdev to device node
    cdev_init(&c_dev, &Fops);
    if (cdev_add(&c_dev, first, 1) == -1)
	{
		device_destroy(cls, first);
		class_destroy(cls);
		unregister_chrdev_region(first, 1);
		return -1;
	}
    
    printk(KERN_INFO "%s The major device number is %d.\n", "Registeration is a success", MAJOR_NUM);
    printk(KERN_INFO "Device file /dev/imu_char dynamically created");

    return 0;
}

/*
 * Cleanup - unregister the appropriate file from /proc
 */
void cleanup_module()
{
    /*
     * Unregister the device
     */
    unregister_chrdev(MAJOR_NUM, DEVICE_NAME);
}

MODULE_DESCRIPTION("Assignment 1, EEE G547");
MODULE_AUTHOR("Naren <h20150547@pilani.bits-pilani.ac.in>");
MODULE_LICENSE("GPL");
MODULE_INFO(ChipSupport, "MPU9255, BMP280");




