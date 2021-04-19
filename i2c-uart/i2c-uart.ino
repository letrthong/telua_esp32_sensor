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

byte _crc8_ccitt_update(byte inCrc, byte inData) {
    uint8_t data = inCrc ^ inData;
    return data;
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
  delay(200);     
  Wire.requestFrom(salveaddress,10 +1); 
  byte checksum =0x00;
  byte crc_out = 0x00;
  byte buffer[10];
  for(int i = 0; i <11; i++){
       while(Wire.available()) { 
        char c = Wire.read(); 
        if(i == 10){
          checksum = c;
        }else{
          buffer[i] = c;
          crc_out = _crc8_ccitt_update(crc_out, c);
        }   
    } 
  }

  if(checksum == crc_out){
    Serial.print("SerialNumber=["); 
    for (int i = 0; i < 10; i++){
        Serial.print(buffer[i] );
    }
    Serial.println("]"); 
  }else{
     Serial.println("crc error");
  }
 
  Serial.println("getSerialNumber::Done");
  delay(1000);   
}
 
void setSerialNumber(String serialNumber){
  Serial.println("setSerialNumber::start serialNumber=" + serialNumber);
  int len = serialNumber.length();
  // Serial.println( len);
  if( len == 10){
    byte type = 3;
    byte cmd = 4;
    byte  crc = type^cmd;

    byte checksum = 0x00;

    for (int i = 0; i < len; i++){
       checksum = _crc8_ccitt_update(checksum, serialNumber[i]);
    }
    
    Wire.beginTransmission(salveaddress);
    Wire.write(type);              
    Wire.write(cmd);              
    Wire.write(crc);              
    Wire.endTransmission();
    delay(100);   
    Wire.beginTransmission(salveaddress);
    Wire.write(serialNumber.c_str(), 10);   
    Wire.write(checksum);                         
    Wire.endTransmission();
    delay(1000);    
  }else{
    Serial.println("setSerialNumber::invalid format");
  }
  Serial.println("setSerialNumber::Done");
}

int getFactoryID(){
  Serial.println("getFactoryID::start");
  byte type = 4;
  byte cmd = 1;
  byte  crc = type^cmd;
     
  Wire.beginTransmission(salveaddress);
  Wire.write(type);              
  Wire.write(cmd);              
  Wire.write(crc);              
  Wire.endTransmission();
   delay(100);   
  Serial.print("getFactoryID=[");      
  Wire.requestFrom(salveaddress,32, 0);  
  while(Wire.available() ) { 
    char c = Wire.read(); 
    Serial.print(c);
  } 
  delay(200);   
  Wire.requestFrom(salveaddress,32 );  
  while(Wire.available() ) { 
    char c = Wire.read(); 
    Serial.print(c);
  } 
  Serial.println("]");
  Serial.println("getFactoryID::Done");
  delay(1000);     
}

int getManuID(){
  Serial.println("getManuID::start");
  byte type = 4;
  byte cmd = 0;
  byte  crc = type^cmd;
     
  Wire.beginTransmission(salveaddress);
  Wire.write(type);              
  Wire.write(cmd);              
  Wire.write(crc);              
  Wire.endTransmission();
  delay(200);   
  Serial.print("getManuID=[");      
  Wire.requestFrom(salveaddress,32);  
  while(Wire.available() ) { 
    char c = Wire.read(); 
    Serial.print(c);
  } 
  Serial.println("]");
  Serial.println("getManuID::Done");
  delay(1000);     
}

void parseCLI(String  input){
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
      }else  if(firstVal.indexOf("off")>=0) {
        digitalWrite(13, LOW);   // switch LED Off
        Serial.write("switch LED Off = 0\n");
      }else if( firstVal.indexOf("i2c-detect")>=0){
          detectI2CSlave();
      }else if(firstVal.indexOf("getSerialNumber")>=0){
          getSerialNumber();
      }else if(firstVal.indexOf("getFactoryID")>=0){
          getFactoryID();
      }else if(firstVal.indexOf("getManuID")>=0){
          getManuID();
      }else {
          Serial.print("Hepler\n");
          Serial.print(" on\n");
          Serial.print(" off\n");
          Serial.print(" i2c-detect\n");
          Serial.print(" getSerialNumber\n");
          Serial.print(" getFactoryID\n");
          Serial.print(" getManuID\n");
      }  
  }else{
    if( firstVal.indexOf("setSerialNumber")>=0){
        setSerialNumber(secondVal);
     }else {
        Serial.print("setSerialNumber 12345678AB\n");
     }   
  }
}

void detectI2CSlave(){
  Serial.println("detectI2CSlave::start");
  int  nDevices ;
  byte error, address;
  nDevices = 0;
  for(address = 1; address < 127; address++){
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
     Serial.println("No I2C devices found");
  }
  Serial.println("detectI2CSlave::Done");
}

void loop() {
  while(Serial.available()) {
      char line[128];
      size_t lineLength = Serial.readBytesUntil('\n', line, 127);
      line[lineLength] = '\0';
      Serial.println("Arduino received=" + String(line));
      delay(100);   
      parseCLI( String(line));
  }
}
