#include <EEPROM.h>



#include <Wire.h>


struct uhes_i2c_msg {
  byte type;
  byte cmd;
  byte crc;
};

int salveaddress = 0x05;

void setup() {
  Wire.begin(); // join I2C bus as the master
  //Wire.beginTransmission(salveaddress);
  //Wire.endTransmission();
  Serial.begin(9600);
  while (!Serial)
  Serial.write("test uart\n");
}

void loop() {
  Serial.println("\nloop...\n");
  byte type = 4;
  byte cmd = 4;
  byte  crc = type^cmd;
       
  Wire.beginTransmission(salveaddress);
  Wire.write(type);              
  Wire.write(cmd);              
  Wire.write(crc);              
  Wire.endTransmission();
 delay(1000);   
 Serial.println("requestFrom...\n");      
 Wire.requestFrom(salveaddress,10);  
 while(Wire.available()>=1) { 
    char c = Wire.read(); 
    Serial.print(c);
 }  
}
