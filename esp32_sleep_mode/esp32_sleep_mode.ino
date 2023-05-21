 #include <WiFi.h>
#include <EEPROM.h>

#include <HTTPClient.h>
#include <ArduinoJson.h>

#include "Adafruit_SHT4x.h"
 

#define uS_TO_S_FACTOR 1000000ULL  /* Conversion factor for micro seconds to seconds */


RTC_DATA_ATTR   int bootCount = 0;



#define EEPROM_SIZE 64
#define TIME_TO_SLEEP  5     

const char* deviceID = "12334332343443ADVED";
String serverName = "http://34.111.197.130:80/service/v1/esp32/update-sensor";
 

int EEPROM_ADDRESS_SSID = 0;
int EEPROM_ADDRESS_PASS = 32;
int EEPROM_ADDRESS_TIME_TO_SLEEP = 64;

bool hasRouter = false;
bool hasSensor = false;
int  time_to_sleep_mode = TIME_TO_SLEEP;

Adafruit_SHT4x sht4 = Adafruit_SHT4x();

void initWiFi() {
  WiFi.mode(WIFI_STA);

  String current_ssid = EEPROM.readString(EEPROM_ADDRESS_SSID);
  String current_pass = EEPROM.readString(EEPROM_ADDRESS_PASS);
  unsigned int lastStringLength = current_ssid.length();

  hasRouter = false;
   

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
      if (lastStringLength > 0) {
        if (current_ssid.equals(SSID)) {
          hasRouter = true;
          break;
        }
      }
    }
  }
  WiFi.scanDelete();

  if (hasRouter == true) {
    WiFi.begin(current_ssid, current_pass);
    Serial.print("Connecting to WiFi ..");
    int count = 0;
    while (WiFi.status() != WL_CONNECTED) {
      Serial.print('.');
      delay(1000);
      count = count + 1;
      if (count > 10) {
        break;
      }
    }

  }

  int count = 0;
  if (WiFi.status() != WL_CONNECTED) {
        WiFi.mode(WIFI_AP_STA);
        WiFi.beginSmartConfig();
        while (!WiFi.smartConfigDone()) {
          delay(500);
          Serial.print(".");
          count  = count + 1;
          if(count >  360){
             ESP.restart();
          }
        }

        Serial.println("");
        Serial.println("SmartConfig received.");

        count  = 0;
        while (WiFi.status() != WL_CONNECTED) {
          delay(500);
          Serial.print(".");
          count  = count + 1;
          if(count >  360){
             ESP.restart();
          }
        }

      String ssid = WiFi.SSID(); 
      String pass = WiFi.psk();
      
      if (ssid.length() > 1 && pass.length() >= 8) {
         Serial.print("SmartConfig readString ssid=");
         Serial.println(ssid);

         Serial.print("SmartConfig readString pass=");
         Serial.println(pass);
  
        EEPROM.writeString(EEPROM_ADDRESS_SSID, ssid);
        EEPROM.commit();
        
        EEPROM.writeString(EEPROM_ADDRESS_PASS, pass);
        EEPROM.commit();
      }
  }
 
  Serial.println(WiFi.localIP());
}

void turnOffWiFi(){
  WiFi.disconnect();
}

void initSht4x() {
  Serial.println("Adafruit SHT4x test");
  if (!sht4.begin()) {
    Serial.println("Couldn't find SHT4x");
  } else {
    hasSensor = true;
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

void initEEPROM() {
  // Allocate The Memory Size Needed
  EEPROM.begin(EEPROM_SIZE);

  int seconds = EEPROM.readUInt(EEPROM_ADDRESS_TIME_TO_SLEEP);
   if(seconds > 5){
       time_to_sleep_mode  = seconds;
   }
}

void sendReport(){
  if(WiFi.status() != WL_CONNECTED){
    return;
  }

   String temperature = "0";
    String relative_humidity ="0";
    if(hasSensor == true){
       sensors_event_t humidity, temp;
      sht4.getEvent( & humidity, & temp);
      
      //      Serial.print("Temperature: ");
      //      Serial.print(temp.temperature);
      //      Serial.println(" degrees C");
      //      Serial.print("Humidity: "); 
      //      Serial.print(humidity.relative_humidity);
      //      Serial.println("% rH");
      
      temperature = String(temp.temperature, 2);
      relative_humidity = String(humidity.relative_humidity, 2);
    }
   

    HTTPClient http;
    String serverPath = serverName + "?sensorName=SHT40&temperature=" + temperature + "&humidity=" + relative_humidity + "&deviceID=" + deviceID;
    http.begin(serverPath.c_str());

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
        Serial.print("deserializeJson intervalTime=");
        int  intervalTime = doc ["intervalTime"];
         Serial.println(intervalTime);
        if(intervalTime>= 30000){
            int seconds =  intervalTime/1000;
            if( time_to_sleep_mode != seconds  && seconds < 600){
                time_to_sleep_mode = seconds;
                EEPROM.writeUInt(EEPROM_ADDRESS_TIME_TO_SLEEP, seconds);  
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
}
/*
Method to print the reason by which ESP32
has been awaken from sleep
*/
void print_wakeup_reason(){
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch(wakeup_reason)
  {
    case ESP_SLEEP_WAKEUP_EXT0 : Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1 : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER : Serial.println("Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP : Serial.println("Wakeup caused by ULP program"); break;
    default : Serial.printf("Wakeup was not caused by deep sleep: %d\n",wakeup_reason); break;
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000); //Take some time to open up the Serial Monitor

  //Increment boot number and print it every reboot
  ++bootCount;
  Serial.println("Boot number: " + String(bootCount));

  
  initEEPROM();

  initWiFi();
   
  initSht4x();

  sendReport();
  
  turnOffWiFi();
  
  //Print the wakeup reason for ESP32
  print_wakeup_reason();

  /*
  First we configure the wake up source
  We set our ESP32 to wake up every 5 seconds
  */
  esp_sleep_enable_timer_wakeup(time_to_sleep_mode * uS_TO_S_FACTOR);
  Serial.println("Setup ESP32 to sleep for every " + String(time_to_sleep_mode) + " Seconds");

  
  Serial.println("Going to sleep now");
  Serial.flush(); 
  esp_deep_sleep_start();
  Serial.println("This will never be printed");
}

void loop(){
  //This is not going to be called
}
