//gcc -Wall -o final_project AutomatedFish.c -lwiringPi -lpthread -lwiringPiDev
#include <stdio.h>
#include <errno.h>
//#include <wiringPi.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h> // for sleep() and usleep()
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>

//Pins for Motor
#define bridge1A  0
#define bridge1B  1
#define bridge2A  2
#define bridge2B  3

#define LCD_RS  4               //Register select pin
#define LCD_E   5               //Enable Pin
#define LCD_D4  6               //Data pin 4
#define LCD_D5  25               //Data pin 5
#define LCD_D6  26              //Data pin 6
#define LCD_D7  27               //Data pin 7

int uart0_filestream = -1;
// Whole Stepping Sequence
int whole[4][4] = {{1,0,0,0},
                   {0,1,0,0},
                   {0,0,1,0},
                   {0,0,0,1}};

void hello(void){
    printf("hello world");
}

void *thread(void *arg) { // arguments not used in this case
    sleep(2); // wait 9 seconds
    usleep(10000); // wait 10000 microseconds (1000000s are 1 second)
    // thread has sleeped for 9.01 seconds
    hello(); // call your function
    // add more here
    return NULL;
}

void SetupUART(void){
    uart0_filestream = open("/dev/ttyAMA0", O_RDWR | O_NOCTTY | O_NDELAY);		//Open in non blocking read/write mode
    if (uart0_filestream == -1){
		//ERROR - CAN'T OPEN SERIAL PORT
		printf("Error - Unable to open UART.  Ensure it is not in use by another application\n");
    }
    struct termios options;
    tcgetattr(uart0_filestream, &options);
    options.c_cflag = B9600 | CS8 | CLOCAL | CREAD;		//<Set baud rate
    options.c_iflag = IGNPAR;
    options.c_oflag = 0;
    options.c_lflag = 0;
    tcflush(uart0_filestream, TCIFLUSH);
    tcsetattr(uart0_filestream, TCSANOW, &options);
}

void GetPH(void){
    if (uart0_filestream != -1){
	// Read up to 255 characters from the port if they are there
	unsigned char rx_buffer[256];
	int rx_length = read(uart0_filestream, (void*)rx_buffer, 255);		//Filestream, buffer to store in, number of bytes to read (max)
	if (rx_length < 0){
		//An error occured (will occur if there are no bytes)
	}
	else if (rx_length == 0){
		//No data waiting
	}
	else{
	    //Bytes received
	    rx_buffer[rx_length] = '\0';
    	    printf("%i bytes read : %s\n", rx_length, rx_buffer);
	}
    }
}


int main(void) {
    time_t rawtime;
    struct tm * timeinfo;
    time ( &rawtime );
    timeinfo = localtime ( &rawtime );


/**
    //Set up wiring pi
    if (wiringPiSetup() == -1) {
        fprintf(stdout, "oops: %s\n", strerror(errno));
        return 1;
    }
    
    //Set up LED display
    if (lcd = lcdInit (2, 16,4, LCD_RS, LCD_E ,LCD_D4 , LCD_D5, LCD_D6,LCD_D7,0,0,0,0)){
            printf ("lcdInit failed! \n");
            return -1 ;
    }
   **/ 
    
    SetupUART();
    pthread_t pt;
    pthread_create(&pt, NULL, thread, NULL);
    printf ( "Current local time and date: %s", asctime (timeinfo) );
    //Main loop
    while(1){
        
    }
    
    return 0;
}


