#include <EEPROM.h>



#include <Wire.h>

void setup() {
  Wire.begin(); // join I2C bus as the master
  
  // put your setup code here, to run once:
  pinMode(13, OUTPUT);      // set LED pin as output
  digitalWrite(13, LOW);    // switch off LED pin
  Serial.begin(9600);
  while (!Serial)
  Serial.write("test uart\n");
}

void loop() {
  delay(500);
  /*while(Serial.available()) {
      char data_rcvd = Serial.read();
      if(data_rcvd == '1'){
        digitalWrite(13, HIGH);  // switch LED On
        Serial.write("  switch LED On");
        Serial.write("1\n");  
        break;
      }
      
      if(data_rcvd == '0') {
        digitalWrite(13, LOW);   // switch LED Off
        Serial.write("  switch LED Off");
        Serial.write("0\n");
        break;
      }
  }*/

    //delay(500);
    Serial.println("Scanning...");
    int  nDevices ;
    byte error, address;
    nDevices = 0;
    for(address = 1; address < 127; address++ )
    {
       
      // The i2c_scanner uses the return value of
      // the Write.endTransmisstion to see if
      // a device did acknowledge to the address.
      Wire.beginTransmission(address);
      error = Wire.endTransmission();
   
      if (error == 0) {
        Serial.print("I2C device found at address 0x");
        if (address<16){
            Serial.print("0");
        }
        Serial.print(address,HEX);
        Serial.println("  !");
   
        nDevices++;
      }else if (error==4){
        Serial.print("Unknown error at address 0x");
        if (address<16){
           Serial.print("0");
        }
        Serial.println(address,HEX);
      }    
    }
    
      if (nDevices == 0){
         Serial.println("No I2C devices found\n");
      }
      else{
        Serial.println("done\n");
      }

       delay(1000);  
    
}
