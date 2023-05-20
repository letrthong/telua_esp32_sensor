#include <WiFi.h>
#include "Adafruit_SHT4x.h"

#include <HTTPClient.h>



const char* ssid = "hcmus";   /*SSID of network to connect*/
const char* password = "fetelxxx";   /*password for SSID*/

const char* ssid1 = "TP-Link_2878";   /*SSID of network to connect*/
const char* password1 = "51521264";   /*password for SSID*/



const char* email = "letrthong@gmail.com";

String serverName = "http://34.111.197.130:80/service/v1/esp32/update-sensor";

unsigned long previousMillis = 0;
unsigned long interval = 30000;


Adafruit_SHT4x sht4 = Adafruit_SHT4x();



void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi ..");
  int count = 0;
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
    count = count +1;
    if(count > 10){
      break;
    }
  }

  if(WiFi.status() != WL_CONNECTED){
      Serial.print("Connecting to WiFix ..");
      WiFi.disconnect();
      WiFi.begin(ssid1, password1);
      delay(1000);
      WiFi.reconnect();
      count = 0;
      while (WiFi.status() != WL_CONNECTED) {
        Serial.print('.');
        delay(1000);
        count = count +1;
        if(count > 10){
          break;
        }
      }
  }

  Serial.println(WiFi.localIP());
}
  
void initSht4x(){
   Serial.println("Adafruit SHT4x test");
  if (! sht4.begin()) {
    Serial.println("Couldn't find SHT4x");
  } else {
        Serial.println("Found SHT4x sensor");
        Serial.print("Serial number 0x");
        Serial.println(sht4.readSerial(), HEX);
      
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
 
void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  initWiFi();
  Serial.print("RSSI (WiFi strength): ");
  Serial.println(WiFi.RSSI());

  initSht4x();
   
}

void loop() {
  // Serial.println("Hello from Telua ESP-WROOM-32");
   delay(1000);
    unsigned long currentMillis = millis();
  /*if condition to check wifi reconnection*/
  if ((WiFi.status() != WL_CONNECTED) && (currentMillis - previousMillis >=interval)) {
    Serial.print(millis());
    Serial.println("  Reconnecting to WiFi...");
    WiFi.disconnect();
    delay(1000);
    WiFi.reconnect();
    previousMillis = currentMillis;
  } else if ((currentMillis - previousMillis >=interval)){
      sensors_event_t humidity, temp;
      
      uint32_t timestamp = millis();
      sht4.getEvent(&humidity, &temp);

//      Serial.print("Temperature: ");
//      Serial.print(temp.temperature);
//      Serial.println(" degrees C");
//      Serial.print("Humidity: "); 
//      Serial.print(humidity.relative_humidity);
//      Serial.println("% rH");

      String temperature =  String(temp.temperature, 2);
       String relative_humidity =  String(humidity.relative_humidity, 2);
      
      HTTPClient http; 
      String serverPath = serverName + "?sensorName=SHT40&temperature=" + temperature + "&humidity=" + relative_humidity ;
      http.begin(serverPath.c_str());
      
      // Send HTTP GET request
      int httpResponseCode = http.GET();
      
      if (httpResponseCode>0) {
//        Serial.print("HTTP Response code: ");
//        Serial.println(httpResponseCode);
        String payload = http.getString();
//        Serial.println(payload);
      } else {
//        Serial.print("Error code: ");
//        Serial.println(httpResponseCode);
      }
      // Free resources
      http.end();
      previousMillis = currentMillis;
  }
}
