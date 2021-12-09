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

int main()
{	
    int i=0;
	int fd;
	unsigned long int empty_spaces;
	
	fd = open(FILE_NAME, O_RDWR | O_SYNC);
	if (fd < 0) {
		perror("Failed to open pir_parking device");
		exit(errno);
	}

    while(i<3000){
		delay(1000); // Keep a delay of 1 second
		read(fd, &empty_spaces, 4);
		printf("\nAvailable Empty Spaces: %d",empty_spaces);
		i++;
	}
	close(fd);
	return EXIT_SUCCESS;
}
