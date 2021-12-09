Title: Smart Parking Lot System Driver for Raspberry Pi 3B 
Tested on Raspberry Pi OS (Release: October 30th 2021), Kernel version 5.10

Summary: This project implements a character device driver for a smart parking lot system consisting of two HC-SR501 PIR sensors. The sensors are interfaced with the Raspberry Pi using physical GPIO pins. One sensor is to be deployed at the entrance (Pin 35/ GPIO 19) and one at the exit (Pin 33/ GPIO 13) of the parking lot. The required connections are shown in "schematic.png". The sensors give a high signal whenever they detect some motion in their vicinity. Depending on the number of vehicles entering/leaving the parking lot, the number of available parking spaces is shown on the screen.


1. Build the project $make
2. Insert Kernel module $sudo insmod pir_parking.ko
3. View Kernel Log to verify that the module is inserted $dmesg
4. Run the userapp $sudo ./userapp
5. Provide the number of currently available parking spaces to the userapp when prompted. 
6. The number of available parking spaces will be continuously updated on the display.
7. Trigger the entry and exit pir sensors to observe the corresponding decrement/increment change in the number of parking spaces available. 
8. Press ctrl+c to stop the userapp
