#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <wiringPiI2C.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#define DEBUG 0

#define OSS 3

// Variables from datasheet
short AC1, AC2, AC3;
unsigned short AC4, AC5, AC6;
short B1, B2, MB, MC, MD;
long B5, UP, UT, temp, pressure;
int fd;


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

int

int main(int argc, char **argv){
  int reg1, reg2, reg3, timing, DEVICE_ADDRESS;
  
  DEVICE_ADDRESS = 0x77; //Default Address of i2c sensor

  //Set up wiring pi and calibrate sensor.
  fd = wiringPiI2CSetup (DEVICE_ADDRESS);
  read_calibration_values(fd);
  
  while(1){
      delay(1000);
      //get raw temperature
      wiringPiI2CWriteReg8 (fd, 0xF4, 0x2E);  
      usleep(4500);
      reg1 = wiringPiI2CReadReg8 (fd, 0xF6);
      reg2 = wiringPiI2CReadReg8 (fd, 0xF7);
      UT = (reg1 << 8) + reg2;

      //get raw pressure
      timing = 13500;
      wiringPiI2CWriteReg8 (fd, 0xF4, 0x34 + (OSS << 6));
      usleep(timing);
      reg1 = wiringPiI2CReadReg8 (fd, 0xF6);
      reg2 = wiringPiI2CReadReg8 (fd, 0xF7);
      reg3 = wiringPiI2CReadReg8 (fd, 0xF8);
      UP = ((reg1 << 16) + (reg2 << 8) + reg3) >> (8 - OSS);
  
  get_readings(UT, UP);
  printf("Temperature: %.1f *C, Pressure: %d Pa\n", (float)temp/10, (int) pressure);
  }

  return 0;
}
