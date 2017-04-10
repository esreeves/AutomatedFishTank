#include <stdlib.h>

//Define Pins for Arduino
int miso  = 8;
int cs = 9;
int sclk  = 10;

char dataString[50] = {0};
const byte pHpin = A0;
const byte tempPin = A2;
float Po;

void setup() {
  Serial.begin(9600);              //Starting serial communication

  //set up spi
  pinMode(cs, OUTPUT);
  pinMode(sclk, OUTPUT); 
  pinMode(miso, INPUT);

  digitalWrite(cs, HIGH);
}

//Read from spi device and return unconverted digits
byte spiread(void) { 
  int i;
  byte d = 0;

  for (i = 7; i >= 0; i--)
  {
    digitalWrite(sclk, LOW);     //Output off
    _delay_ms(1);
    if (digitalRead(miso)) {
      //set the bit to 0 no matter what
      d |= (1 << i);
    }

    digitalWrite(sclk, HIGH);     //Output on
    _delay_ms(1);
  }

  return d;
}


//Convert raw celcius to readable
double readCelsius(void) {
  uint16_t v;

  // Slave select 
  digitalWrite(cs, LOW);
  _delay_ms(1);
  v = spiread();
  v <<= 8; //
  v |= spiread(); //or the output
  digitalWrite(cs, HIGH);

  if (v & 0x4) {
    return NAN; //Could not detect sensor
  }
  v >>= 3;
  return v*0.25;
}


void loop() {
  Po = (1023 - analogRead(pHpin)) / 73.07;   // Read and reverse the analogue input value from the pH sensor then scale 0-14.
  sprintf(dataString,"%02X",Po); // convert a value to hexa 
  Serial.print("PH Level: ");
  Serial.print(Po, 2);                    // Print the result in the serial monitor.
  
  Serial.print("\t Degrees Celsius: ");
  Serial.println(readCelsius());
  delay(2000);                  // give the loop some break
}


