/*
 *  chardev2.h - the header file with the ioctl definitions.
 *
 *  The declarations here have to be in a header file, because
 *  they need to be known both to the kernel module
 *  (in chardev.c) and the process calling ioctl (ioctl.c)
 */

#ifndef IMUDEV_H
#define IMUDEV_H

#include <linux/ioctl.h>

/*
 * The major device number. We can't rely on dynamic
 * registration any more, because ioctls need to know
 * it.
 */
#define MAJOR_NUM 101
#define MINOR_NUM 1

/*
 * Set the message of the device driver
 */
#define IOCTL_SET_MSG _IOW(MAJOR_NUM, 0, char *)
/*  User space to kernel space
 * _IOW means that we're creating an ioctl command
 * number for passing information from a user process
 * to the kernel module.
 *
 * The first arguments, MAJOR_NUM, is the major device
 * number we're using.
 *
 * The second argument is the number of the command
 * (there could be several with different meanings).
 *
 * The third argument is the type we want to get from
 * the process to the kernel.
 */

/*
 * Get the message of the device driver
 */
 
#define IOCTL_GET_MSG _IOR(MAJOR_NUM, 1, char *)
/* Kernel space to user space
 * This IOCTL is used for output, to get the message
 * of the device driver. However, we still need the
 * buffer to place the message in to be input,
 * as it is allocated by the process.
 */

/*
 * Get the n'th byte of the message
 */
#define IOCTL_GET_NTH_BYTE _IOWR(MAJOR_NUM, 2, int)
/*
 * The IOCTL is used for both input and output. It
 * receives from the user a number, n, and returns
 * Message[n].
 */

/* Self Calibrate sensors (__IO Type IOCTL Call) */
#define IOCTL_CALIBRATE_MPU9255 _IO(MAJOR_NUM, 3)
#define IOCTL_CALIBRATE_BMP280 _IO(MAJOR_NUM, 4)

/* 
Change Sensor Range & Resolution (Assuming that there are a few few options available, which can be specified with an integer value)
(__IOW Type IOCTL Call)/
*/ 
#define IOCTL_GYRO_RR _IOW(MAJOR_NUM, 5, int)
#define IOCTL_ACCEL_RR _IOW(MAJOR_NUM, 6, int)
#define IOCTL_COMP_RR _IOW(MAJOR_NUM, 7, int)
#define IOCTL_PRESSURE_RR _IOW(MAJOR_NUM, 8, int)


/*
Adding required IOCTLs to read the sensor values
(__IOR Type IOCTL Call)/
*/ 
#define IOCTL_GET_GYRO _IOR(MAJOR_NUM, 9, unsigned long)
#define IOCTL_GET_ACCEL _IOR(MAJOR_NUM, 10, unsigned long)
#define IOCTL_GET_COMP _IOR(MAJOR_NUM, 11, unsigned long)
#define IOCTL_GET_PRESSURE _IOR(MAJOR_NUM, 12, unsigned long)

 /*
 * The name of the device file
 */ 
#define DEVICE_FILE_NAME "/dev/imu_char"

#endif
