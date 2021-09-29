/*
 *  ioctl.c - the process to use ioctl's to control the kernel module
 *
 *  Until now we could have used cat for input and output.  But now
 *  we need to do ioctl's, which require writing our own process.
 */

/*
 * device specifics, such as ioctl numbers and the
 * major device file.
 */
#include "imudev.h"

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>              /* open */
#include <unistd.h>             /* exit */
#include <sys/ioctl.h>          /* ioctl */

/*
 * Functions for the ioctl calls
 */

int ioctl_set_msg(int file_desc, char *message)
{
    int ret_val;

    ret_val = ioctl(file_desc, IOCTL_SET_MSG, message);

    if (ret_val < 0) {
        printf("ioctl_set_msg failed:%d\n", ret_val);
        exit(-1);
    }
    return 0;
}

int ioctl_get_msg(int file_desc)
{
    int ret_val;
    char message[100];

    /*
     * Warning - this is dangerous because we don't tell
     * the kernel how far it's allowed to write, so it
     * might overflow the buffer. In a real production
     * program, we would have used two ioctls - one to tell
     * the kernel the buffer length and another to give
     * it the buffer to fill
     */
    ret_val = ioctl(file_desc, IOCTL_GET_MSG, message);

    if (ret_val < 0) {
        printf("ioctl_get_msg failed:%d\n", ret_val);
        exit(-1);
    }

    printf("get_msg message:%s\n", message);
    return 0;
}

int ioctl_get_nth_byte(int file_desc)
{
    //int i;
    int c;

    printf("get_nth_byte message:");

    //i = 0;
    //do {
        c = ioctl(file_desc, IOCTL_GET_NTH_BYTE,0);

        if (c < 0) {
            printf("ioctl_get_nth_byte failed\n");
            exit(-1);
        }

        printf("Second = %d \n\n",c);
    //} while (c != 0);
    putchar('\n');
    return 0;
}

/*
 * Main - Call the ioctl functions
 */
 
int ioctl_get_gyro(int file_desc)
{
    int ret_val;
    unsigned long message;

    /*
     * Warning - this is dangerous because we don't tell
     * the kernel how far it's allowed to write, so it
     * might overflow the buffer. In a real production
     * program, we would have used two ioctls - one to tell
     * the kernel the buffer length and another to give
     * it the buffer to fill
     */
    ret_val = ioctl(file_desc, IOCTL_GET_GYRO, &message);

    if (ret_val < 0) {
        printf("ioctl_get_accel failed:%d\n", ret_val);
        exit(-1);
    }
    u_int16_t gyro_16_bit = (u_int16_t) message;

    //Using a random demonstrative example to separate different bits from the obtained value to get the readings of different axes. 
    printf("\n\nGyroscope 16 bit reading = : %d\n", gyro_16_bit);
    printf("Gyroscope x axis reading = : %f rads per sec\n", (float)gyro_16_bit/12500);
    printf("Gyroscope y axis reading = : %f rads per sec\n", (float)gyro_16_bit/11100);
    printf("Gyroscope y axis reading = : %f rads per sec\n", (float)gyro_16_bit/14000);
    printf("Gyroscope status code = : %d\n", ((0xF000 & gyro_16_bit)>>12)%5);
    return 0;
}

int ioctl_get_accel(int file_desc)
{
    int ret_val;
    unsigned long message;

    /*
     * Warning - this is dangerous because we don't tell
     * the kernel how far it's allowed to write, so it
     * might overflow the buffer. In a real production
     * program, we would have used two ioctls - one to tell
     * the kernel the buffer length and another to give
     * it the buffer to fill
     */
    ret_val = ioctl(file_desc, IOCTL_GET_ACCEL, &message);

    if (ret_val < 0) {
        printf("ioctl_get_accel failed:%d\n", ret_val);
        exit(-1);
    }
    u_int16_t accel_16_bit = (u_int16_t) (2*message);

    //Using a random demonstrative example to separate different bits from the obtained value to get the readings of different axes. 
    printf("\nAccelerometer 16 bit reading = : %d\n", accel_16_bit);
    printf("Accelerometer x axis reading = : %f m/s^2\n", (float)accel_16_bit/8500);
    printf("Accelerometer y axis reading = : %f m/s^2\n", (float)accel_16_bit/7700);
    printf("Accelerometer y axis reading = : %f m/s^2\n", (float)accel_16_bit/8500);
    printf("Accelerometer status code = : %d\n", ((0xF000 & accel_16_bit)>>12)%5);
    return 0;
}

int ioctl_get_comp(int file_desc)
{
    int ret_val;
    unsigned long message;

    /*
     * Warning - this is dangerous because we don't tell
     * the kernel how far it's allowed to write, so it
     * might overflow the buffer. In a real production
     * program, we would have used two ioctls - one to tell
     * the kernel the buffer length and another to give
     * it the buffer to fill
     */
    ret_val = ioctl(file_desc, IOCTL_GET_COMP, &message);

    if (ret_val < 0) {
        printf("ioctl_get_comp failed:%d\n", ret_val);
        exit(-1);
    }
    u_int16_t comp_16_bit = (u_int16_t) (3*message);

    //Using a random demonstrative example to separate different bits from the obtained value to get the readings of different axes. 
    printf("\nCompass 16 bit reading = : %d\n", comp_16_bit);
    printf("Compass x axis reading = : %d degrees\n", (int)(comp_16_bit*2.3)%180);
    printf("Compass y axis reading = : %d degrees\n", (int)(comp_16_bit*1.6)%180);
    printf("Compass y axis reading = : %d degrees\n", (int)(comp_16_bit*3.4)%180);
    printf("Compass status code = : %d\n", ((0xF000 & comp_16_bit)>>12)%5);
    return 0;
}

int ioctl_get_pressure(int file_desc)
{
    int ret_val;
    unsigned long message;
    int x;

    /*
     * Warning - this is dangerous because we don't tell
     * the kernel how far it's allowed to write, so it
     * might overflow the buffer. In a real production
     * program, we would have used two ioctls - one to tell
     * the kernel the buffer length and another to give
     * it the buffer to fill
     */
    ret_val = ioctl(file_desc, IOCTL_GET_PRESSURE, &message);

    if (ret_val < 0) {
        printf("ioctl_get_pressure failed:%d\n", ret_val);
        exit(-1);
    }

    //Using a random demonstrative example to separate different bits from the obtained value to get the readings of different axes. 
    printf("\nBMP280 20 bit reading = : %d\n", x = 0x000000000000FFFF & 4*message);
    printf("BMP280 Pressure Reading = : %f Pascal \n", (float)x/115);
    return 0;
}

int ioctl_calibrate_BMP280(int file_desc)
{
    int ret_val;
    ret_val = ioctl(file_desc, IOCTL_CALIBRATE_BMP280);

    if (ret_val < 0) {
        printf("ioctl_calibrate_BMP280 failed:%d\n", ret_val);
        exit(-1);
    }

    printf("\n\nBMP280 Calibration Complete");
    return 0;
}

int ioctl_calibrate_MPU9255(int file_desc)
{
    int ret_val;
    ret_val = ioctl(file_desc, IOCTL_CALIBRATE_MPU9255);

    if (ret_val < 0) {
        printf("ioctl_calibrate_MPU9255 failed:%d\n", ret_val);
        exit(-1);
    }

    printf("\n\nMPU9255 Calibration Complete");
    return 0;
}

int ioctl_gyro_rr(int file_desc, int setting)
{
    int ret_val;
    ret_val = ioctl(file_desc, IOCTL_GYRO_RR, setting);

    if (ret_val < 0) {
        printf("ioctl_gyro_rr failed: %d\n", ret_val);
        exit(-1);
    }
    printf("\n\nMPU9255 Gyro Range and Resolution Changed to Setting %d",setting);
    return 0;
}

int ioctl_accel_rr(int file_desc, int setting)
{
    int ret_val;
    ret_val = ioctl(file_desc, IOCTL_ACCEL_RR, setting);

    if (ret_val < 0) {
        printf("ioctl_accel_rr failed: %d\n", ret_val);
        exit(-1);
    }
    printf("\n\nMPU9255 Accel Range and Resolution Changed to Setting %d",setting);
    return 0;
}

int ioctl_comp_rr(int file_desc, int setting)
{
    int ret_val;
    ret_val = ioctl(file_desc, IOCTL_COMP_RR, setting);

    if (ret_val < 0) {
        printf("ioctl_COMP_rr failed: %d\n", ret_val);
        exit(-1);
    }
    printf("\n\nMPU9255 COMP Range and Resolution Changed to Setting %d",setting);
    return 0;
}

int ioctl_pressure_rr(int file_desc, int setting)
{
    int ret_val;
    ret_val = ioctl(file_desc, IOCTL_PRESSURE_RR, setting);

    if (ret_val < 0) {
        printf("ioctl_pressure_rr failed: %d\n", ret_val);
        exit(-1);
    }
    printf("\n\nBMP280 Pressure Range and Resolution Changed to Setting %d",setting);
    return 0;
}


int main()
{

    printf("Welcome to the imu_char user application. \n");
    printf("Compile the project using '$ make'. \n");
    printf("Insert kernel module using '$ sudo insmod main.ko'. \n");
    printf("Character device file /dev/imu_char will be automatically created. \n");    
//    printf("Create character device file using '$ sudo mknod /dev/imu_char c 101 0' \n");
    printf("Once the above steps are done, this application will calibrate the sensors, set default range and resolution settings, and get one measurement on each of the 10 axes. \n");
    printf("Kernel operations can be seen using $dmesg.");
        printf("\n\n\n############ Application Started ############\n");
    int file_desc, ret_val;
    char *msg = "Message passed by ioctl\n";
    file_desc = open(DEVICE_FILE_NAME, 0);
    if (file_desc < 0) {
        printf("Can't open device file: %s. Run userapp with sudo: '$ sudo ./userapp' OR change file permission using '$ sudo chmod 777 /dev/imu_char' and run '$ ./userapp' again \n", DEVICE_FILE_NAME);
        exit(-1);
    }

//    ioctl_get_nth_byte(file_desc);
//   ioctl_get_msg(file_desc);
//    ioctl_set_msg(file_desc, "Heil Mogambo\n\n");

// First Calibrate the sensors. 
      ret_val = ioctl_calibrate_MPU9255(file_desc);  
      ret_val = ioctl_calibrate_BMP280(file_desc); 

// // Set the required resolution and range for each sensor
      ret_val = ioctl_gyro_rr(file_desc,5);
      ret_val = ioctl_accel_rr(file_desc, 4);
      ret_val = ioctl_comp_rr(file_desc, 2);    
      ret_val = ioctl_pressure_rr(file_desc, 2);  

// Get the required sensor measurements
      ret_val = ioctl_get_gyro(file_desc);
      ret_val = ioctl_get_accel(file_desc);
      ret_val = ioctl_get_comp(file_desc);
      ret_val = ioctl_get_pressure(file_desc);  
  
    
    close(file_desc);
    return 0;
}
