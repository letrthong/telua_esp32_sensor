 #include <WiFi.h>

 

 #include <ArduinoJson.h>
 
 // https://github.com/Sensirion/arduino-i2c-scd4x
#include <SensirionI2CScd4x.h>
#include <Wire.h>
 

// the LED is connected to GPIO 5
const int ledBlue =  5; 
const int ledGreen =  17; 
const int ledYellow =  16; 
const int ledRed=  4; 
bool hasSensor = false;
int led_index = 0;
  
 
SensirionI2CScd4x scd4x;



void intGpio(){
    pinMode(ledBlue, OUTPUT);
    digitalWrite(ledBlue, LOW);
    
    pinMode(ledGreen, OUTPUT);
    digitalWrite(ledGreen, LOW);
    
    pinMode(ledYellow, OUTPUT);
    digitalWrite(ledYellow, LOW);
    
    pinMode(ledRed, OUTPUT);
    digitalWrite(ledRed, LOW);
}

void turnOnNotification(int led){
   if( led == 0){
       digitalWrite(ledBlue, HIGH);
       delay(1000);
       digitalWrite(ledGreen, LOW);
   }else  if( led == 1){
      digitalWrite(ledGreen, HIGH);
      delay(1000);
      
      digitalWrite(ledBlue, LOW);
   }else if( led == 2){
        digitalWrite(ledYellow, HIGH);
        delay(500);
        digitalWrite(ledYellow, LOW);
        delay(500);
         
      digitalWrite(ledGreen, LOW);
      digitalWrite(ledBlue, LOW);
   }else if( led == 3){
      digitalWrite(ledRed, HIGH);
      delay(500);
      digitalWrite(ledRed, LOW);
      delay(500);

      digitalWrite(ledGreen, LOW);
      digitalWrite(ledBlue, LOW);
   }
   
}
void printUint16Hex(uint16_t value) {
    Serial.print(value < 4096 ? "0" : "");
    Serial.print(value < 256 ? "0" : "");
    Serial.print(value < 16 ? "0" : "");
    Serial.print(value, HEX);
}

void printSerialNumber(uint16_t serial0, uint16_t serial1, uint16_t serial2) {
    Serial.print("Serial: 0x");
    printUint16Hex(serial0);
    printUint16Hex(serial1);
    printUint16Hex(serial2);
    Serial.println();
}

 
 void initSht4x() {
      Wire.begin();

    uint16_t error;
    char errorMessage[256];

    scd4x.begin(Wire);

    // stop potentially previously started measurement
    error = scd4x.stopPeriodicMeasurement();
    if (error) {
        Serial.print("Error trying to execute stopPeriodicMeasurement(): ");
        errorToString(error, errorMessage, 256);
        Serial.println(errorMessage);
    }

    uint16_t serial0;
    uint16_t serial1;
    uint16_t serial2;
    error = scd4x.getSerialNumber(serial0, serial1, serial2);
    if (error) {
        Serial.print("Error trying to execute getSerialNumber(): ");
        errorToString(error, errorMessage, 256);
        Serial.println(errorMessage);
    } else {
        printSerialNumber(serial0, serial1, serial2);
    }

    // Start Measurement
    error = scd4x.startPeriodicMeasurement();
    if (error) {
        Serial.print("Error trying to execute startPeriodicMeasurement(): ");
        errorToString(error, errorMessage, 256);
        Serial.println(errorMessage);
    }

    Serial.println("Waiting for first measurement... (5 sec)");
 }

  

 void readSensor() {
     uint16_t error;
    char errorMessage[256];

    delay(100);

    // Read Measurement
    uint16_t co2 = 0;
    float temperature = 0.0f;
    float humidity = 0.0f;
    uint16_t  isDataReady = false;
    error = scd4x.getDataReadyStatus(isDataReady);
    if (error) {
        Serial.print("Error trying to execute getDataReadyFlag(): ");
        errorToString(error, errorMessage, 256);
        Serial.println(errorMessage);
        return;
    }
    if (!isDataReady) {
        return;
    }
    error = scd4x.readMeasurement(co2, temperature, humidity);
    if (error) {
        Serial.print("Error trying to execute readMeasurement(): ");
        errorToString(error, errorMessage, 256);
        Serial.println(errorMessage);
    } else if (co2 == 0) {
        Serial.println("Invalid sample detected, skipping.");
    } else {
        Serial.print("Co2:");
        Serial.print(co2);
        led_index = co2;
        Serial.print("\t");
        Serial.print("Temperature:");
        Serial.print(temperature);
        Serial.print("\t");
        Serial.print("Humidity:");
        Serial.println(humidity);
       if( humidity < 10){
           ESP. restart(); 
        }
    }
 }
  

 void setup() {
   Serial.begin(115200);
   delay(1000);  
   intGpio();
   initSht4x();
 
 }

void processSensor(){
    readSensor();
    if(led_index > 1600){
      turnOnNotification(3);
    } else if(led_index > 1000){
        turnOnNotification(2);
    } else if(led_index > 800){
      turnOnNotification(1);
    } else{
      turnOnNotification(0);
    } 
}


 void loop() {
    processSensor();
 }
