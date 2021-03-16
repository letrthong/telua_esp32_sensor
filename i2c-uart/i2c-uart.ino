#include <EEPROM.h>

#include <CommandParser.h>

#include <Wire.h>


struct uhes_i2c_msg {
  byte type;
  byte cmd;
  byte crc;
};

int salveaddress = 0x05;

void setup() {
  Wire.begin(); // join I2C bus as the master

  // put your setup code here, to run once:
  pinMode(13, OUTPUT);      // set LED pin as output
  digitalWrite(13, LOW);    // switch off LED pin

  
  Serial.begin(9600);
  while (!Serial)
  Serial.write("test uart\n");
}

int getSerialNumber(){
  Serial.println("getSerialNumber::start");
  byte type = 4;
  byte cmd = 4;
  byte  crc = type^cmd;
       
  Wire.beginTransmission(salveaddress);
  Wire.write(type);              
  Wire.write(cmd);              
  Wire.write(crc);              
  Wire.endTransmission();
 delay(500);   
 Serial.print("SerialNumber=[");      
 Wire.requestFrom(salveaddress,10);  
 while(Wire.available()>=1) { 
    char c = Wire.read(); 
    Serial.print(c);
 } 
  Serial.println("]\n");
 Serial.println("getSerialNumber::Done");
 delay(100);   
}


void setSerialNumber(String serialNumber){
   Serial.println("setSerialNumber::start serialNumber=" + serialNumber);
   Serial.println("setSerialNumber::Done");
}

void parseCML(String  input){
  String  firstVal = input;
  String  secondVal;
  bool isGetInfo = true;
  for (int i = 0; i < input.length(); i++) {
    if (input.substring(i, i+1) == " ") {
        firstVal = input.substring(0, i) ;
        secondVal = input.substring(i+1);
        secondVal.replace(" ", "");
        isGetInfo = false;
        break;
    }
  }

  if(isGetInfo == true){
      if( firstVal.indexOf("on")>=0){
        digitalWrite(13, HIGH);  // switch LED On
        Serial.write("switch LED On=1\n");
      }  else  if(firstVal.indexOf("off")>=0) {
        digitalWrite(13, LOW);   // switch LED Off
        Serial.write("switch LED Off = 0\n");
      } else if( firstVal.indexOf("i2c-detect")>=0){
          detectI2CSlave();
       }else if(firstVal.indexOf("getSerialNumber")>=0){
          getSerialNumber();
       } else {
          Serial.print("Hepler\n");
          Serial.print(" on\n");
          Serial.print(" off\n");
          Serial.print(" i2c-detect\n");
          Serial.print(" getSerialNumber\n");
       } 
  }else{
     if( firstVal.indexOf("setSerialNumber")>=0){
         setSerialNumber(secondVal);
      }else {
          Serial.println("key=" + firstVal);
          Serial.println("Value=" + secondVal);
      }   
  }
}

void detectI2CSlave(){
  Serial.println("detectI2CSlave::start\n");
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
    } else{
      Serial.println("done\n");
    }

    Serial.println("detectI2CSlave::Done\n");
}

void loop() {
  while(Serial.available()) {
      char line[128];
      size_t lineLength = Serial.readBytesUntil('\n', line, 127);
      line[lineLength] = '\0';
      Serial.println(line);
      parseCML( String(line));
  }
}
