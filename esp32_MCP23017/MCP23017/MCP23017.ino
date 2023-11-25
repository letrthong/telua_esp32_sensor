//http://www.esp32learning.com/code/esp32-and-mcp23017-flashy-led-example.php
// https://github.com/keriszafir/mcp23017-demo/blob/master/simple-on-off.c

#include "Wire.h"
 
 //#define IODIRA 0x00
 //#define IODIRB 0x01
 //#define GPIOA 0x12
 // #define GPIOB 0x13
void setup()
{
  Wire.begin(); // wake up I2C bus
  // set I/O pins to outputs
  Wire.beginTransmission(0x20);
  Wire.write(0x00); // IODIRA register
  Wire.write(0x00); // set all of port A to outputs
  Wire.endTransmission();


  Wire.beginTransmission(0x20);
  Wire.write(0x01); // IODIRB register
  Wire.write(0x00); // set all of port B to outputs
  Wire.endTransmission();
  
}
 
void loop()
{
  Wire.beginTransmission(0x20);
  Wire.write(0x12); // address bank A
  Wire.write((byte)0xAA); // value to send - all HIGH
  Wire.endTransmission();
  delay(5000);

  Wire.beginTransmission(0x20);
  Wire.write(0x12); // address bank A
  Wire.write((byte)0x55); // value to send - all HIGH
  Wire.endTransmission();
  delay(5000);

   Wire.beginTransmission(0x20);
  Wire.write(0x13); // address bank B
  Wire.write((byte)0xAA); // value to send - all HIGH
  Wire.endTransmission();
  delay(5000);


     Wire.beginTransmission(0x20);
  Wire.write(0x13); // address bank B
  Wire.write((byte)0x55); // value to send - all HIGH
  Wire.endTransmission();
  delay(5000);
}