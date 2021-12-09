#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>

#define FILE_NAME "/dev/pir_parking"
#define MSG_LEN 2
#define BUF_SIZE (MSG_LEN + 1)

 void delay(int number_of_seconds) 
 { 
      /* Convert time to ms */ 
     int milli_seconds = 1000 * number_of_seconds; 
  
      /* Store start time */
     clock_t start_time = clock(); 
  
     /* Loop for required duration */
     while (clock() < start_time + milli_seconds) 
         ; 
 }

clock_t entry_time[1000];
clock_t exit_time[1000];

int main()
{	
    int i=0;
	int fd;
	char val[2];
	int available_spaces;
	int car_number = 0;
	
	fd = open(FILE_NAME, O_RDWR | O_SYNC);
	if (fd < 0) {
		perror("Failed to open pir_parking device");
		exit(errno);
	}
	printf("\nEnter the number of available parking spaces");
	scanf("%d",&available_spaces);

    while(i<3000){
		delay(1000); // Keep a delay of 1 second
		read(fd, val, 1);
		
		if (val[0] == 2)
			{
				printf("\nPIR_SENSOR_2: Car Exit Detected");
				available_spaces++;
				delay(4000); /*The sensor needs 4 seconds to go from HIGH to LOW*/
			}
		else if(val[0] == 1)
			{
				printf("\nPIR_SENSOR_1: Car Entry Detected");
				available_spaces--;
				car_number++;
				delay(4000); /*The sensor needs 4 seconds to go from HIGH to LOW*/
				
			}
		
		printf("\nAvailable Empty Spaces: %d",available_spaces);
		i++;
	}
	close(fd);
	return EXIT_SUCCESS;
}
