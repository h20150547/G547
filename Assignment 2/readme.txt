Please perform the following steps to test this dof block driver.	

1. $ make  // Build the project. 
2. $ sudo insmod main.ko  // Insert the driver module into the kernel.
3. $ lsmod // View the inserted module. 
4. $ ls -l /dev/  //View the newly added partitions
5. $ sudo dd if=/dev/dof of=dof // Dump the contents of input file dof into output file dof (in present working directory).
6. $ sudo chmod 777 /dev/dof1 // Enable all permissions for dof1.
7. $ sudo cat > /dev/dof1  // Write required data into dof1. Press Ctrl+C to exit. 
8. $ sudo xxd /dev/dof1 | less // Display contents of dof1
9. $ sudo fdisk -l /dev/dof OR ls -l /dev/dof* // See partition information
10. $ sudo rmmod main.ko // Remove the driver module
