#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/init.h>
#include <asm/io.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include <linux/fs.h>
#include <linux/module.h>

#define BCM2835_PERI_BASE 0x3f000000   
#define GPIO_BASE (BCM2835_PERI_BASE + 0x200000)
#define GPIO_REGION_SIZE 0x3c
#define GPLEV_OFFSET 0x34

/* Detect PIR signal at two GPIO Pins */

#define GPIO_PIR1 19  // This is GPIO 19/ Pin 35 -> Car Entry
#define GPIO_PIR2 13 // This is GPIO 13/ Pin 33 -> Car Exit

#define HIGH 1
#define LOW 0
#define MODULE_NAME "pir_parking"
#define NUM_PIR_Sensors 2
#define BUF_SIZE 2

char val[2];

/* Define the required operations on the device file*/
static ssize_t pir_parking_read(struct file *filep, char __user *buf, size_t len, loff_t *off);
static ssize_t pir_parking_write(struct file *filep, const char __user *buf, size_t len, loff_t *off);
static int pir_parking_open(struct inode *inodep, struct file *filep);
static int pir_parking_release(struct inode *inodep, struct file *filep);

/* GPIO Related functions*/

/* Save GPIO Configuration */
static void PIR1_GPIO_FUNC_SAVE(void);
static void PIR2_GPIO_FUNC_SAVE(void);

/* Restore GPIO Configuration */
static void PIR1_GPIO_FUNC_RESTORE(void);
static void PIR2_GPIO_FUNC_RESTORE(void);

/* Configure GPIO as INPUT */
static void PIR1_GPIO_INPUT(void);
static void PIR2_GPIO_INPUT(void);

/* Read GPIO Pins */
static void PIR1_READ(void);
static void PIR2_READ(void);

/* Character Device Related */
static dev_t devt;
static struct cdev pir_parking_cdev;
static struct class *pir_parking_class;
static struct device *pir_parking_device;

/* 
  iomap variable to point to the first GPIO Register 
 */
static void __iomem *iomap;

/* Variables to store GPIO register offsets */
static int PIR1_GPIO_FUNC_REG_OFFSET;
static int PIR2_GPIO_FUNC_REG_OFFSET;

/* Variables to store GPIO bit offsets */
static int PIR1_GPIO_FUNC_BIT_OFFSET;
static int PIR2_GPIO_FUNC_BIT_OFFSET;

/* Buffers to store GPIO data */
static char GPIO_PIR1_VAL[BUF_SIZE] = "";
static char GPIO_PIR2_VAL[BUF_SIZE] = "";

/* Variables to store GPIO initial configurations */
static int PIR1_GPIO_FUNC_INIT;
static int PIR2_GPIO_FUNC_INIT;

/* File Operations Structure */
static struct file_operations fops = {
	.owner = THIS_MODULE,
	.open = pir_parking_open,
	.release = pir_parking_release,
	.read = pir_parking_read,
	.write = pir_parking_write,
};

/* Variables for GPIO Numbers */
static int PIR1_GPIO_NUM = GPIO_PIR1;
static int PIR2_GPIO_NUM = GPIO_PIR2;

/* Init Module */
static int __init pir_parking_pir(void)
{
	int ret;
	
		ret = alloc_chrdev_region(&devt, 0, NUM_PIR_Sensors, MODULE_NAME);    /* Allocate Memory for New Driver */
	if (ret) {
		pr_err("%s: Failed to allocate char device region.\n",
			MODULE_NAME);
		goto out;
	}

	cdev_init(&pir_parking_cdev, &fops);               /* Link fops and pir_parking_cdev */
	pir_parking_cdev.owner = THIS_MODULE;
	ret = cdev_add(&pir_parking_cdev, devt, NUM_PIR_Sensors);
	if (ret) {
		pr_err("%s: Failed to add cdev.\n", MODULE_NAME);
		goto cdev_err;
	}

	pir_parking_class = class_create(THIS_MODULE, "pir-test");    /* Create Device Class */
	if (IS_ERR(pir_parking_class)) {
		pr_err("%s: class_create() failed.\n", MODULE_NAME);
		ret = PTR_ERR(pir_parking_class);
		goto class_err;
	}

	pir_parking_device = device_create(pir_parking_class, NULL, devt, NULL, MODULE_NAME);      /* Create Device */
	if (IS_ERR(pir_parking_device)) {
		pr_err("%s: device_create() failed.\n", MODULE_NAME);
		ret = PTR_ERR(pir_parking_device);
		goto dev_err;
	}

	iomap = ioremap(GPIO_BASE, GPIO_REGION_SIZE);           /* Map Bus memory to CPU Space*/ 
	if (!iomap) {
		pr_err("%s: ioremap() failed.\n", MODULE_NAME);
		ret = -EINVAL;
		goto remap_err;
	}

	/* Calculate Function Select Register Offset */
	PIR1_GPIO_FUNC_REG_OFFSET = 4 * (PIR1_GPIO_NUM / 10);
	PIR2_GPIO_FUNC_REG_OFFSET = 4 * (PIR2_GPIO_NUM / 10); 

	/* Calculate bit offset for function selection */
	PIR1_GPIO_FUNC_BIT_OFFSET = (PIR1_GPIO_NUM % 10) * 3; 
	PIR2_GPIO_FUNC_BIT_OFFSET = (PIR2_GPIO_NUM % 10) * 3;

	/* Save Initial/Default GPIO Configuration */
	PIR1_GPIO_FUNC_SAVE();     
	PIR2_GPIO_FUNC_SAVE();

	/* Confgiure GPIO Direction as Input */
	PIR1_GPIO_INPUT();  
	PIR2_GPIO_INPUT();

	/* Kernel Loaded Message */
	pr_info("%s: Module loaded\n", MODULE_NAME);
	goto out;

remap_err:
	device_destroy(pir_parking_class, devt);
dev_err:
	class_unregister(pir_parking_class);
	class_destroy(pir_parking_class);
class_err:
	cdev_del(&pir_parking_cdev);
cdev_err:
	unregister_chrdev_region(devt, NUM_PIR_Sensors);
out:
	return ret;
}

static void __exit exit_parking_pir(void)
{
	PIR1_GPIO_FUNC_RESTORE();
	PIR2_GPIO_FUNC_RESTORE();
	iounmap(iomap);
	device_destroy(pir_parking_class, devt);
	class_unregister(pir_parking_class);
	class_destroy(pir_parking_class);
	cdev_del(&pir_parking_cdev);
	unregister_chrdev_region(devt, NUM_PIR_Sensors);
	pr_info("%s: Module unloaded\n", MODULE_NAME);
}

static int pir_parking_open(struct inode *inodep, struct file *filep)
{
	return 0;
}

static int pir_parking_release(struct inode *inodep, struct file *filep)
{
	return 0;
}

static ssize_t
pir_parking_read(struct file *filep, char __user *buf, size_t len, loff_t *off)
{
	int err;
	/* Read PIR Sensor outputs*/

	PIR1_READ(); 
	PIR2_READ();
	
	if ((int)GPIO_PIR1_VAL[0]==49)    /* PIR 1 Triggered: Send 1, Car Entered */
	{ 	val[0] = 1;
			printk("Car movement at PIR 1");
			printk("GPIO_PIR1_VAL[0] = %s\n",GPIO_PIR1_VAL);
		err = copy_to_user(buf, val, 1);
		printk("%d",err);
	if (err)
		return -EFAULT;
	}
	else if ((int)GPIO_PIR2_VAL[0]==49)      /* PIR 1 Triggered: Send 1, Car Exited */
	{ val[0] = 2;
	printk("Car movement at PIR 2");
	printk("GPIO_PIR2_VAL[0] = %s\n",GPIO_PIR2_VAL);
		err = copy_to_user(buf, val, 1);
	if (err)
		return -EFAULT;
	}
	else {
		val[0]=0;
		printk("No car movement"); 			 /* No car movement: Send 0 */
		err = copy_to_user(buf,val,1);
		if (err)
		return -EFAULT;
		}

	return 0;
}

static ssize_t
pir_parking_write(struct file *filep, const char __user *buf, size_t len, loff_t *off)
{ 
	return 0;
}



static void PIR1_GPIO_FUNC_SAVE(void)                 /* Initial function triplet saved*/
{
	int val;

	val = ioread32(iomap + PIR1_GPIO_FUNC_REG_OFFSET);
	PIR1_GPIO_FUNC_INIT = (val >> PIR1_GPIO_FUNC_BIT_OFFSET) & 7;
}

static void PIR2_GPIO_FUNC_SAVE(void)
{
	int val;

	val = ioread32(iomap + PIR2_GPIO_FUNC_REG_OFFSET);
	PIR2_GPIO_FUNC_INIT = (val >> PIR2_GPIO_FUNC_BIT_OFFSET) & 7;
}

static void PIR1_GPIO_FUNC_RESTORE(void)          /* Initial function triplet restored*/
{
	int val;

	val = ioread32(iomap + PIR1_GPIO_FUNC_REG_OFFSET);
	val &= ~(7 << PIR1_GPIO_FUNC_BIT_OFFSET);
	val |= PIR1_GPIO_FUNC_INIT << PIR1_GPIO_FUNC_BIT_OFFSET;
	iowrite32(val, iomap + PIR1_GPIO_FUNC_REG_OFFSET);
}

static void PIR2_GPIO_FUNC_RESTORE(void)
{
	int val;

	val = ioread32(iomap + PIR2_GPIO_FUNC_REG_OFFSET);
	val &= ~(7 << PIR2_GPIO_FUNC_BIT_OFFSET);
	val |= PIR2_GPIO_FUNC_INIT << PIR2_GPIO_FUNC_BIT_OFFSET;
	iowrite32(val, iomap + PIR2_GPIO_FUNC_REG_OFFSET);
}

static void PIR1_GPIO_INPUT(void)			/*Setting the GPIO to input (FUNCTION BITS = 000) */
{
	int val;

	val = ioread32(iomap + PIR1_GPIO_FUNC_REG_OFFSET);
	val &= ~(6 << PIR1_GPIO_FUNC_BIT_OFFSET);
	val |= 0 << PIR1_GPIO_FUNC_BIT_OFFSET;
	iowrite32(val, iomap + PIR1_GPIO_FUNC_REG_OFFSET);
}

static void PIR2_GPIO_INPUT(void)
{
	int val;

	val = ioread32(iomap + PIR2_GPIO_FUNC_REG_OFFSET);
	val &= ~(6 << PIR2_GPIO_FUNC_BIT_OFFSET);
	val |= 0 << PIR2_GPIO_FUNC_BIT_OFFSET;
	iowrite32(val, iomap + PIR2_GPIO_FUNC_REG_OFFSET);
}


static void PIR1_READ(void)				 /* GPLEV - read only register used to check value of GPIOs */ 
{
	int val;

	val = ioread32(iomap + GPLEV_OFFSET);  
	val = (val >> PIR1_GPIO_NUM) & 1;
	GPIO_PIR1_VAL[0] = val ? '1' : '0';
}

static void PIR2_READ(void)
{
	int val;

	val = ioread32(iomap + GPLEV_OFFSET);
	val = (val >> PIR2_GPIO_NUM) & 1;
	GPIO_PIR2_VAL[0] = val ? '1' : '0';
}

module_init(pir_parking_pir);
module_exit(exit_parking_pir);

MODULE_DESCRIPTION("Smart Parking Lot System Driver for Raspberry Pi 3B");
MODULE_AUTHOR("Naren (2015HD400547P) and Nishad (2016HS400215P)");
MODULE_LICENSE("GPL");
MODULE_INFO(ChipSupport, "2 x HC-SR501");
