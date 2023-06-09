 #include <WiFi.h>

 #include <EEPROM.h>

 #include <WiFiClientSecure.h>

 #include <HTTPClient.h>

 #include <ArduinoJson.h>

 #include "Adafruit_SHT4x.h"
#include "Adafruit_SGP40.h"

 #define uS_TO_S_FACTOR 1000000ULL /* Conversion factor for micro seconds to seconds */
 #define EEPROM_SIZE 256
 #define TIME_TO_SLEEP 30


RTC_DATA_ATTR int bootCount = 0;
RTC_DATA_ATTR bool isCorrectPassword = false;


 String deviceID = "";
 String serialNumber = "";
 String serverName = "https://telua.co/service/v1/esp32/update-sensor";

 int EEPROM_ADDRESS_SSID = 0;
 int EEPROM_ADDRESS_PASS = 32;
 int EEPROM_ADDRESS_TIME_TO_SLEEP = 64;
 int EEPROM_ADDRESS_DEVICE_ID = 128;
 int EEPROM_ADDRESS_SERIAL_NUMBER = 192;

unsigned long previousMillis = 0;
unsigned long interval = 30000;

 bool hasRouter = false;
 bool hasSensor = false;
 int time_to_sleep_mode = TIME_TO_SLEEP;

Adafruit_SHT4x sht4 = Adafruit_SHT4x();
Adafruit_SGP40 sgp;

// the LED is connected to GPIO 5
const int ledPin =  5; 
const int buzzerPin =  17; 


void intGpio(){
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);

  pinMode(buzzerPin, OUTPUT);
  digitalWrite(buzzerPin, LOW);
  
}

void turnOffLed(){
    Serial.println("turnOffLed");
    digitalWrite(ledPin, LOW);
}


void turnOnBuzzer(){
  Serial.println("turnOnBuzzer");
   digitalWrite(buzzerPin, HIGH);
}


void turnOffBuzzer(){
    Serial.println("turnOffBuzzer");
    digitalWrite(buzzerPin, LOW);
}


void turnOnLed(){
  Serial.println("turnOnLed");
   digitalWrite(ledPin, HIGH);
}



 void initWiFi() {
   WiFi.mode(WIFI_STA);

   String current_ssid = EEPROM.readString(EEPROM_ADDRESS_SSID);
   String current_pass = EEPROM.readString(EEPROM_ADDRESS_PASS);
   unsigned int  length_of_ssid  = current_ssid.length();
   g_ssid = current_ssid;
   hasRouter = false;

    if(isCorrectPassword == false){
        for(int y = 0; y< 3; y++){
        ssid_list = "";
        int n = WiFi.scanNetworks();
        Serial.println("Scan done");
         if (n == 0) {
           Serial.println("no networks found");
         } else {
           Serial.print(n);
           Serial.println(" networks found");
           for (int i = 0; i < n; ++i) {
             String SSID = WiFi.SSID(i);
             Serial.print("scanNetworks SSID=");
             Serial.println(SSID);
             if( i < 5){
                  ssid_list = ssid_list + SSID +  ",";
              }
              
             if (length_of_ssid > 0) {
               if (current_ssid.equals(SSID)) {
                 hasRouter = true;
                  Serial.println("scanNetworks hasRouter");
                 break;
               }
             }
           }
      
           int len = ssid_list.length();
           if(len >1){
              ssid_list = ssid_list.substring(0,len-1);
           }
         }
         WiFi.scanDelete();
  
         if(hasRouter == true){
          break;
         }
         delay(500);
     }
    }
   

   if (hasRouter == true || isCorrectPassword == true) {
     WiFi.begin(current_ssid, current_pass);
     Serial.println("Connecting to WiFi ..");
     int countWifiStatus = 0;
     int retryTime = 30;
     if(isCorrectPassword == true){
        retryTime = 60;
     }
     while (WiFi.status() != WL_CONNECTED) {
       Serial.print('.');
       delay(500);
        // 15 seconds
       countWifiStatus = countWifiStatus + 1;
       if (countWifiStatus  > retryTime  ) {
         break;
       }
        
     }

     if(WiFi.status() == WL_CONNECTED){
        isCorrectPassword = true;
     }
   }
    Serial.println(WiFi.localIP());
   if (WiFi.status() != WL_CONNECTED && isCorrectPassword == false){
      startLocalWeb();
  } 
 }

 void turnOffWiFi() {
   WiFi.disconnect();
 }

 void initSht4x() {
   Serial.println("Telua SHT4x test");
    
  if (! sgp.begin()){
    Serial.println("SGP40 sensor not found :(");
    return;
  }else{ 
        Serial.print("Found SGP40 serial #");
        Serial.print(sgp.serialnumber[0], HEX);
        Serial.print(sgp.serialnumber[1], HEX);
        Serial.println(sgp.serialnumber[2], HEX);
  }

   
   if (!sht4.begin()) {
     Serial.println("Couldn't find SHT4x");
   } else {
     hasSensor = true;
     Serial.println("Found SHT4x sensor");
     Serial.print("Serial number 0x");
     Serial.println(sht4.readSerial(), HEX);
     if (bootCount < 2) {
       // You can have 3 different precisions, higher precision takes longer
       sht4.setPrecision(SHT4X_HIGH_PRECISION);
       switch (sht4.getPrecision()) {
       case SHT4X_HIGH_PRECISION:
         Serial.println("High precision");
         break;
       case SHT4X_MED_PRECISION:
         Serial.println("Med precision");
         break;
       case SHT4X_LOW_PRECISION:
         Serial.println("Low precision");
         break;
       }
       switch (sht4.getHeater()) {
       case SHT4X_NO_HEATER:
         Serial.println("No heater");
         break;
       case SHT4X_HIGH_HEATER_1S:
         Serial.println("High heat for 1 second");
         break;
       case SHT4X_HIGH_HEATER_100MS:
         Serial.println("High heat for 0.1 second");
         break;
       case SHT4X_MED_HEATER_1S:
         Serial.println("Medium heat for 1 second");
         break;
       case SHT4X_MED_HEATER_100MS:
         Serial.println("Medium heat for 0.1 second");
         break;
       case SHT4X_LOW_HEATER_1S:
         Serial.println("Low heat for 1 second");
         break;
       case SHT4X_LOW_HEATER_100MS:
         Serial.println("Low heat for 0.1 second");
         break;
       }
     }
   }
 }

 void initEEPROM() {
   // Allocate The Memory Size Needed
   EEPROM.begin(EEPROM_SIZE);

   int seconds = EEPROM.readUInt(EEPROM_ADDRESS_TIME_TO_SLEEP);
   if (seconds > TIME_TO_SLEEP) {
     time_to_sleep_mode = seconds;
   }

   Serial.print("initEEPROM time_to_sleep_mode=");
   Serial.println(time_to_sleep_mode);

   deviceID = EEPROM.readString(EEPROM_ADDRESS_DEVICE_ID);
   Serial.print("initEEPROM deviceID=");
   Serial.println(deviceID);

   serialNumber = EEPROM.readString(EEPROM_ADDRESS_SERIAL_NUMBER);
   Serial.print("initEEPROM serialNumber=");
   Serial.println(serialNumber);
 }

 void sendReport(bool hasReport) {
   
   String temperature = "0";
   String relative_humidity = "0";
   String  str_voc_index = "0";
   if (hasSensor == true) {
     sensors_event_t humidity, temp;
     sht4.getEvent( & humidity, & temp);
    
     

     temperature = String(temp.temperature, 2);
     relative_humidity = String(humidity.relative_humidity, 2);

     // https://github.com/adafruit/Adafruit_SGP40
      int32_t voc_index;
      uint16_t sraw;
      float t  = temp.temperature;
       Serial.print("Temp *C = "); Serial.print(t); Serial.print("\t\t");

       float h = humidity.relative_humidity;
       Serial.print("Hum. % = "); Serial.println(h);
  
      sraw = sgp.measureRaw(t,  h);
      Serial.print("Raw measurement: ");
      Serial.println(sraw);
      
      voc_index = sgp.measureVocIndex( t, h);
      Serial.print("Voc Index: ");
      Serial.println(voc_index);
      str_voc_index = String(voc_index, 2);
      
   }
    
   if ( hasReport == false) {
     return;
   }

    initWiFi();
    if (WiFi.status() != WL_CONNECTED  ) {
     return;
   }
   WiFiClientSecure * client = new WiFiClientSecure;
   if (!client) {
    turnOffWiFi()
     return;
   }

   client -> setInsecure();
   HTTPClient http;
   String serverPath = serverName + "?sensorName=SHT40-SGP40&temperature=" + temperature + "&humidity=" + relative_humidity +"&voc_index="+ str_voc_index  +  + "&deviceID=" + deviceID + "&serialNumber=" + serialNumber;

   http.begin( *client, serverPath.c_str());

   // Send HTTP GET request
   int httpResponseCode = http.GET();

   if (httpResponseCode == 200) {
     //        Serial.print("HTTP Response code: ");
     //        Serial.println(httpResponseCode);
     String payload = http.getString();
     //      Serial.println(payload);

     //https://arduinojson.org/v6/doc/upgrade/
     DynamicJsonDocument doc(1024);

     DeserializationError error = deserializeJson(doc, payload);
     if (error) {
       Serial.println("deserializeJson() failed");

     } else {

       int intervalTime = doc["intervalTime"];
       Serial.print("deserializeJson intervalTime=");
       Serial.println(intervalTime);
       if (intervalTime >= 30) {

         if (time_to_sleep_mode != intervalTime && intervalTime <= 1800) {
           time_to_sleep_mode = intervalTime;
           EEPROM.writeUInt(EEPROM_ADDRESS_TIME_TO_SLEEP, intervalTime);
           EEPROM.commit();
         }
       }

       if (deviceID.length() != 32) {
         String id = doc["deviceID"];
         Serial.print("deserializeJson deviceID=");
         Serial.println(id);
         if (id.length() > 1 && id.length() < 64) {
           EEPROM.writeString(EEPROM_ADDRESS_DEVICE_ID, id);
           EEPROM.commit();
         }

         String serial_number = doc["serialNumber"];
         if (serial_number.length() > 1 && serial_number.length() < 64) {
           EEPROM.writeString(EEPROM_ADDRESS_SERIAL_NUMBER, serial_number);
           EEPROM.commit();
         }
       }
     }

   } else {
     //        Serial.print("Error code: ");
     //        Serial.println(httpResponseCode);
   }
   // Free resources
   http.end();
   delete client;

   turnOffWiFi();
 }
 /*
 Method to print the reason by which ESP32
 has been awaken from sleep
 */
 
 void setup() {
   Serial.begin(115200);
   delay(1000); //Take some time to open up the Serial Monitor

   //Increment boot number and print it every reboot
   ++bootCount;
   Serial.println("Boot number: " + String(bootCount));

   initEEPROM();

   intGpio();
   
   initSht4x();

   sendReport(false);

    
 }

 void loop() {
   //This is not going to be called
      for(int i= 0; i < time_to_sleep_mode ; i++){
          sendReport(false);

          for(int y= 0; y< 5; y++){
            turnOnLed();
             delay(50);
            turnOffLed();
             delay(150);
          }
           
      }

      sendReport(true);
      delay(1000);
 }
