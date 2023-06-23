 #include <WiFi.h>

 #include <EEPROM.h>

 #include <WiFiClientSecure.h>

 #include <HTTPClient.h>

 #include <ArduinoJson.h>

 #include "Adafruit_SHT4x.h"
#include "Adafruit_SGP40.h"
 
Adafruit_SHT4x sht4 = Adafruit_SHT4x();
Adafruit_SGP40 sgp;

// the LED is connected to GPIO 5
const int ledBlue =  5; 
const int ledGreen =  17; 
const int ledYellow =  16; 
const int ledRed=  4; 
bool hasSensor = false;
int led_index = 0;

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
       delay(500);
        digitalWrite(ledBlue, LOW);
   }else  if( led == 1){
       digitalWrite(ledGreen, HIGH);
        delay(500);
        digitalWrite(ledGreen, LOW);
   }else  if( led == 2){
       digitalWrite(ledYellow, HIGH);
         delay(500);
        digitalWrite(ledYellow, LOW);
   }else  if( led == 3){
      digitalWrite(ledRed, HIGH);
       delay(500);
       digitalWrite(ledRed, LOW);
   }
    delay(500);
}
 
 
 
 void initSht4x() {
   Serial.println("Telua SHT4x test");
    
  if (! sgp.begin()){
    Serial.println("SGP40 sensor not found :(");
     delay(1000);
     ESP. restart();
    return;
  }else{ 
        Serial.print("Found SGP40 serial #");
        Serial.print(sgp.serialnumber[0], HEX);
        Serial.print(sgp.serialnumber[1], HEX);
        Serial.println(sgp.serialnumber[2], HEX);
  }

   
   if (!sht4.begin()) {
     Serial.println("Couldn't find SHT4x");
      delay(1000);
      ESP. restart(); 
   } else {
     hasSensor = true;
     Serial.println("Found SHT4x sensor");
     Serial.print("Serial number 0x");
     Serial.println(sht4.readSerial(), HEX);
//     if (bootCount < 2) {
//       // You can have 3 different precisions, higher precision takes longer
//       sht4.setPrecision(SHT4X_HIGH_PRECISION);
//       switch (sht4.getPrecision()) {
//       case SHT4X_HIGH_PRECISION:
//         Serial.println("High precision");
//         break;
//       case SHT4X_MED_PRECISION:
//         Serial.println("Med precision");
//         break;
//       case SHT4X_LOW_PRECISION:
//         Serial.println("Low precision");
//         break;
//       }
//       switch (sht4.getHeater()) {
//       case SHT4X_NO_HEATER:
//         Serial.println("No heater");
//         break;
//       case SHT4X_HIGH_HEATER_1S:
//         Serial.println("High heat for 1 second");
//         break;
//       case SHT4X_HIGH_HEATER_100MS:
//         Serial.println("High heat for 0.1 second");
//         break;
//       case SHT4X_MED_HEATER_1S:
//         Serial.println("Medium heat for 1 second");
//         break;
//       case SHT4X_MED_HEATER_100MS:
//         Serial.println("Medium heat for 0.1 second");
//         break;
//       case SHT4X_LOW_HEATER_1S:
//         Serial.println("Low heat for 1 second");
//         break;
//       case SHT4X_LOW_HEATER_100MS:
//         Serial.println("Low heat for 0.1 second");
//         break;
//       }
//     }
   }
 }

  
 void readSensor() {
   String temperature = "0";
   String relative_humidity = "0";
   String  str_voc_index = "0";
   if (hasSensor == true) {
      sensors_event_t humidity, temp;
      sht4.getEvent( & humidity, & temp);
      
      if( humidity.relative_humidity < 10){
        ESP. restart(); 
      }
      
      temperature = String(temp.temperature, 2);
      relative_humidity = String(humidity.relative_humidity, 2);
      
      // https://github.com/adafruit/Adafruit_SGP40
       
      uint16_t sraw;
      float t  = temp.temperature;
      Serial.print("Temp *C = "); Serial.print(t); Serial.print("\t\t");
      
      float h = humidity.relative_humidity;
      Serial.print("Hum. % = "); Serial.println(h);
  
      sraw = sgp.measureRaw(t,  h);
      Serial.print("Raw measurement: ");
      Serial.println(sraw);
      
      int32_t   voc_index = sgp.measureVocIndex( t, h);
      Serial.print("Voc Index: ");
      Serial.println(voc_index);
      led_index = voc_index;
      str_voc_index = String(voc_index, 2);  
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
    if(led_index > 400){
      turnOnNotification(3);
    }
    else if(led_index > 200){
        turnOnNotification(2);
    }
    else if(led_index > 100){
      turnOnNotification(1);
    }else{
      turnOnNotification(0);
    }
    delay(1000);
}

void loop() {
   processSensor();
}
