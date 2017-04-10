//gcc -Wall -o final_project AutomatedFish.c -lwiringPi -lpthread -lwiringPiDev
//Basic C library stuff
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <errno.h>          //Error Handling
#include <time.h>           
#include <pthread.h>        //For Sleeping for 24 hours 
#include <unistd.h> //for sleep() and usleep()
#include <fcntl.h>          //File Stuff
#include <lcd.h>//LCD headers from WiringPi
// Wiring PI Headers
#include <wiringSerial.h>
#include <wiringPiI2C.h>
#include <wiringPi.h>
#include <softPwm.h>

#define LCD_RS  4               //Register select pin
#define LCD_E   5               //Enable Pin
#define LCD_D4  6               //Data pin 4
#define LCD_D5  26               //Data pin 5
#define LCD_D6  27              //Data pin 6
#define LCD_D7  28               //Data pin 7

#define STATISTICSBUTTON    25
#define FEEDBUTTON          7

#define DELAY   5
#define DEBUG   0
#define OSS     3
#define CHUNK   5

////////////////////////// - I2C temperture Sensor Variables - //////////////////////////

// Variables from datasheet
short AC1, AC2, AC3;
unsigned short AC4, AC5, AC6;
short B1, B2, MB, MC, MD;
long B5, UP, UT, temp, pressure;
int fd;

int reg1, reg2, reg3, timing;
int DEVICE_ADDRESS = 0x77; //Default Address of i2c sensor

////////////////////////// - Variables For Stepper Motor - //////////////////////////

// Rotation after each button press
volatile int rotation_size = 64;

// 4 indicates whole stepping, 8 indicates half stepping
volatile int step_number = 4;

// Which pins are controlling the stepper position
int pins[4] = {0,1,2,3};

// Whole Stepping Sequence
int whole[4][4] = {{1,0,0,0},
                   {0,1,0,0},
                   {0,0,1,0},
                   {0,0,0,1}};
            
//////////////////////////////////////////////////////////////////////////////


//Calibrate for temperture sensor and set registers of sensor.
void read_calibration_values(int fd){
  AC1 = (wiringPiI2CReadReg8 (fd, 0xAA) << 8) + wiringPiI2CReadReg8 (fd, 0xAB);
  AC2 = (wiringPiI2CReadReg8 (fd, 0xAC) << 8) + wiringPiI2CReadReg8 (fd, 0xAD);
  AC3 = (wiringPiI2CReadReg8 (fd, 0xAE) << 8) + wiringPiI2CReadReg8 (fd, 0xAF);
  AC4 = (wiringPiI2CReadReg8 (fd, 0xB0) << 8) + wiringPiI2CReadReg8 (fd, 0xB1);
  AC5 = (wiringPiI2CReadReg8 (fd, 0xB2) << 8) + wiringPiI2CReadReg8 (fd, 0xB3);
  AC6 = (wiringPiI2CReadReg8 (fd, 0xB4) << 8) + wiringPiI2CReadReg8 (fd, 0xB5);
  B1  = (wiringPiI2CReadReg8 (fd, 0xB6) << 8) + wiringPiI2CReadReg8 (fd, 0xB7);
  B2  = (wiringPiI2CReadReg8 (fd, 0xB8) << 8) + wiringPiI2CReadReg8 (fd, 0xB9);
  MB  = (wiringPiI2CReadReg8 (fd, 0xBA) << 8) + wiringPiI2CReadReg8 (fd, 0xBB);
  MC  = (wiringPiI2CReadReg8 (fd, 0xBC) << 8) + wiringPiI2CReadReg8 (fd, 0xBD);
  MD  = (wiringPiI2CReadReg8 (fd, 0xBE) << 8) + wiringPiI2CReadReg8 (fd, 0xBF);
  if ( DEBUG == 1 ) printf("AC1 = %d \nAC2 = %d \nAC3 = %d \nAC4 = %d \nAC5 = %d \nAC6 = %d \nB1 = %d \nB2 = %d \nMB = %d \nMC = %d \nMD = %d \n", 
     AC1, AC2, AC3, AC4, AC5, AC6, B1, B2, MB, MC, MD);
}


//Getting Temperture reading and then conversion for pressure from datasheet
void get_readings(long UT, long UP){
  long X1, X2, X3, B3, B6, B7, p;
  unsigned long B4;
  X1 = ((UT - AC6) * AC5) >> 15;
  X2 = (MC << 11) / (X1 + MD);
  B5 = X1 + X2;
  temp = (B5 + 8) >> 4;
  
  B6 = B5 - 4000;
  X1 = (B2 * ((B6 * B6) >> 12)) >> 11;
  X2 = (AC2 * B6) >> 11;
  X3 = X1 + X2;
  B3 = (((AC1*4+X3) << OSS) + 2) >> 2;
  X1 = (AC3 * B6) >> 13;
  X2 = (B1 * (B6 * B6 >> 12)) >> 16;
  X3 = ((X1 + X2) + 2) >> 2;
  B4 = (AC4 * (unsigned long)(X3 + 32768)) >> 15;
  B7 = ((unsigned long)UP - B3) * (50000 >> OSS);
  p = B7 < 0x80000000 ? (B7 * 2) / B4 : (B7 / B4) * 2;

  X1 = (p >> 8) * (p >> 8);
  X1 = (X1 * 3038) >> 16;
  X2 = (-7357 * p) >> 16;
  pressure = p + ((X1 + X2 + 3791) >> 4);
}


// Make call to i2c register to read raw data and pass to conversion.
void GetTempererture(void){
    delay(1000);
    //get raw temperature
    wiringPiI2CWriteReg8 (fd, 0xF4, 0x2E);  
    usleep(4500);
    //Read registers for temperture
    reg1 = wiringPiI2CReadReg8 (fd, 0xF6);
    reg2 = wiringPiI2CReadReg8 (fd, 0xF7);
    UT = (reg1 << 8) + reg2;

    //get raw pressure
    timing = 13500;
    wiringPiI2CWriteReg8 (fd, 0xF4, 0x34 + (OSS << 6));
    usleep(timing); //Give i2c time to write
    //Read from 3 registers needed for pressure reading
    reg1 = wiringPiI2CReadReg8 (fd, 0xF6);
    reg2 = wiringPiI2CReadReg8 (fd, 0xF7);
    reg3 = wiringPiI2CReadReg8 (fd, 0xF8);
    UP = ((reg1 << 16) + (reg2 << 8) + reg3) >> (8 - OSS);

    get_readings(UT, UP);
    printf("Temperature: %.1f Degrees Celcius, Pressure: %d pascals\n", (float)temp/10, (int) pressure);
}            


//Read from PH log file.
void GetPH(void){
// open file
    char buf[CHUNK];
    FILE *file;
    size_t nread;

    file = fopen("PH_log.txt", "r");
    if (file) {
        while ((nread = fread(buf, 1, sizeof buf, file)) > 0)
        fwrite(buf, 1, nread, stdout);
        if (ferror(file)) {
        /* deal with error */
        }
    fclose(file);
    }
}


//Rotate the feeder once with the stepper motor.
void FishFeeder(void){
    int turns, steps, pins;
    int i = 0;
    for(i = 0; i < 8; i++){
        // Rotate based on internal gear size
        for(turns = 0; turns < rotation_size; turns++){
            //  Here the step_number
            for(steps = 0; steps < step_number; steps++){
                // Set 4 stepper motor pins
                for(pins = 0; pins < 4; pins++){
                    digitalWrite(pins, whole[steps][pins]);      
                }
            // Delay after all pins have been set
            delay(DELAY);
            }
        }
    }
}


void* AutomaticFeed(void* argument) {
    //sleep(86400); // Wait 24 hours
    sleep(10); // Wait 24 hours
    printf ("Feeding Auto\n");
    FishFeeder();
    AutomaticFeed(NULL);
    return NULL;
}


//Init and value printing for methods
int main(void){
    int lcd;
    
    pthread_t feedThread;
    pthread_create(&feedThread, NULL, AutomaticFeed, NULL);
  
    // Set up wiring pi
    if (wiringPiSetup() == -1) {
        printf("Error Enabling WiringPI");
        return -1;
    }
    
    // Set up LED display
    if ((lcd = lcdInit (2, 16,4, LCD_RS, LCD_E, LCD_D4, LCD_D5, LCD_D6, LCD_D7,0,0,0,0))){
        printf ("lcdInit failed\n");
        return 0;
    }
    
    // STATISTICSBUTTON ISR
    if(wiringPiISR(STATISTICSBUTTON, INT_EDGE_FALLING, &FishFeeder) == -1){
        printf("Error with STATISTICSBUTTON ISR");
        return -1;
    }
    
    fd = wiringPiI2CSetup (DEVICE_ADDRESS); //Set up i2c
    read_calibration_values(fd); //Calibrate Sensor

    // Set Stepper pins for output
    int i;
    for(i = 0; i < 4; i++){
        pinMode(i, OUTPUT);
        printf("Setting Rotation pin: ");
        printf("%d\n", i);
    }
    
    while(1){
        GetPH();
        GetTempererture();
        delay(5000);
    }
}
