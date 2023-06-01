#include <WiFi.h>

 #include <EEPROM.h>

 #include <WiFiClientSecure.h>

 #include <HTTPClient.h>

 #include <ArduinoJson.h>

#include "Adafruit_SHT31.h"


 #define uS_TO_S_FACTOR 1000000ULL /* Conversion factor for micro seconds to seconds */
 #define EEPROM_SIZE 512
 #define TIME_TO_SLEEP 30

 RTC_DATA_ATTR int bootCount = 0;
 
 String deviceID = "";
 String serialNumber = "";
 String configTrigger = "";
 String serverName = "https://telua.co/service/v1/esp32/update-sensor";
 String error_url = "https://telua.co/service/v1/esp32/error-sensor";
 String trigger_url = "https://telua.co/service/v1/esp32/trigger-sensor";
 
 int EEPROM_ADDRESS_SSID = 0;
 int EEPROM_ADDRESS_PASS = 32;
 int EEPROM_ADDRESS_TIME_TO_SLEEP = 64;
 int EEPROM_ADDRESS_DEVICE_ID = 128;
 int EEPROM_ADDRESS_SERIAL_NUMBER = 192;
 int EEPROM_ADDRESS_TRIGGER = 256;

 bool hasRouter = false;
 bool hasSensor = false;
 bool hasError = true;

 int time_to_sleep_mode = TIME_TO_SLEEP;

Adafruit_SHT31 sht3x = Adafruit_SHT31();

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
       delay(500);
        // 15 seconds
       count = count + 1;
       if (count > 30  ) {
         break;
       }
     }
   }

   int count = 0;
   if (WiFi.status() != WL_CONNECTED) {
     Serial.println("beginSmartConfig");
     
     WiFi.mode(WIFI_AP_STA);
     WiFi.beginSmartConfig();
     
     while (!WiFi.smartConfigDone()) {
       delay(500);
       Serial.print(".");
        // 180seconds = 3 minutes 
       count = count + 1;
       if (count > 360) {
         ESP.restart();
       }
     }

     Serial.println("");
     Serial.println("SmartConfig received.");

     count = 0;
     while (WiFi.status() != WL_CONNECTED) {
       delay(500);
       Serial.print(".");
       count = count + 1;
       // 180seconds = 3 minutes 
       if (count > 360) {
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

 void turnOffWiFi() {
   WiFi.disconnect();
 }

 void initSht4x() {
   Serial.println("Telua SHT3x test");
   if (!sht3x.begin(0x44)) {
     Serial.println("Couldn't find SHT4x");
   } else {
     hasSensor = true;
     Serial.println("Found SHT3x sensor");
     Serial.print("Heater Enabled State: ");
      if (sht3x.isHeaterEnabled()){
        Serial.println("ENABLED");
      }
      else{
         Serial.println("DISABLED");
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

   configTrigger = EEPROM.readString(EEPROM_ADDRESS_TRIGGER);
   Serial.print("initEEPROM configTrigger=");
   Serial.println(configTrigger);
 }

 void sendReport() {
   if (WiFi.status() != WL_CONNECTED) {
     return;
   }

   String temperature = "0";
   String relative_humidity = "0";
   float fHumidity = 0.0;
   float fTemperature = 0.0;
   if (hasSensor == true) {
     for (int i = 0; i < 3; i++) {
        // https://github.com/adafruit/Adafruit_SHT31/blob/master/examples/SHT31test/SHT31test.ino
        float t = sht3x.readTemperature();
        float h = sht3x.readHumidity();
         if (! isnan(t)) { 
             temperature = String(t, 2);
         }
  
         if (! isnan(h)) { 
            if(h> 0){
               relative_humidity = String(h, 2);
               hasError = false;
               break;
            }
         }
       delay(500);
     }
   }

   
   WiFiClientSecure * client = new WiFiClientSecure;
   if (!client) {
     return;
   }

   String strTriggerParameter = "";
   //process trigger
   if (configTrigger.length() > 1 && hasSensor == true) {
     StaticJsonDocument < 1024 > docTrigger;

     // parse a JSON array
     DeserializationError errorTrigger = deserializeJson(docTrigger, configTrigger);

     if (errorTrigger) {
       Serial.println("deserializeJson() failed");
       strTriggerParameter ="ConfigError";
     } else {
       // extract the values
       JsonArray triggerList = docTrigger.as < JsonArray > ();
       bool hasTrigger = false;
       for (JsonObject v: triggerList) {
         String property = v["property"];
         Serial.print("property=");
         Serial.println(property);

         String opera = v["operator"];
         Serial.print("operator=");
         Serial.println(opera);

         float value = v["value"];
         Serial.print("value=");
         Serial.println(value);

         String action = v["action"];
         Serial.print("action=");
         Serial.println(action);

         hasTrigger = false;
         float currentValue = 0;
         if (property == "temperature") {
           currentProperty = fTemperature;
         } else if (property == "humidity") {
           currentProperty = fHumidity;
         }

         if (opera == "=") {
           if (currentValue == value) {
              hasTrigger = true;
           }
         } else if (opera == "<") {
           if (currentValue  < value ) {
              hasTrigger = true;
           }
         } else if (opera == ">") {
           if (currentValue > value) {
              hasTrigger = true;
           }
         }else if (opera == ">=") {
           if (currentValue >= value) {
              hasTrigger = true;
           }
         }else if (opera == "<=") {
           if (currentValue <= value) {
              hasTrigger = true;
           }
         }else if (opera == "!=") {
           if (currentValue != value) {
              hasTrigger = true;
           }
         }

         if(hasTrigger == true){
             strTriggerParameter = strTriggerParameter + action + "-";
             //@Turn on off led
         }
       }
     }
   } 

   client -> setInsecure();
   HTTPClient http;
   String serverPath = serverName + "?sensorName=SHT30&temperature=" + temperature + "&humidity=" + relative_humidity + "&deviceID=" + deviceID + "&serialNumber=" + serialNumber;

  if(strTriggerParameter.length() > 0){
    serverPath = trigger_url + "?deviceID=" + deviceID + "&temperature=" + temperature + "&humidity=" + relative_humidity  +  +"&trigger=" + strTriggerParameter;   
  }
    
   if (hasError == true) {
     serverPath = error_url + "?sensorName=SHT30&deviceID=" + deviceID + "&serialNumber=" + serialNumber;
   }

 
   http.begin( * client, serverPath.c_str());

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

       bool hasIntervalTime = doc.containsKey("intervalTime");
       if (hasIntervalTime == true) {
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
       }

       if (deviceID.length() != 32) {
         bool hasDeviceID = doc.containsKey("deviceID");
         if (hasDeviceID == true) {
           String id = doc["deviceID"];
           Serial.print("deserializeJson deviceID=");
           Serial.println(id);
           if (id.length() > 1 && id.length() < 64) {
             EEPROM.writeString(EEPROM_ADDRESS_DEVICE_ID, id);
             EEPROM.commit();
           }
         }

         bool hasSerialNumber = doc.containsKey("serialNumber");
         if (hasSerialNumber == true) {
           String serial_number = doc["serialNumber"];
           if (serial_number.length() > 1 && serial_number.length() < 64) {
             EEPROM.writeString(EEPROM_ADDRESS_SERIAL_NUMBER, serial_number);
             EEPROM.commit();
           }
         }
       }

       //Process Trigger
       bool hasTriggers = doc.containsKey("triggers");
       if (hasTriggers == true) {
         JsonArray triggerList = doc["triggers"];
         //            for(JsonObject v : triggerList) {
         //                String property = v["property"];
         //                 Serial.print("property=");
         //                 Serial.println(property);
         //            }

         String strTrigger = "";
         serializeJson(triggerList, strTrigger);
         Serial.print("strTrigger=");
         Serial.println(strTrigger);
         if (configTrigger != strTrigger) {
           configTrigger = strTrigger;
           if (configTrigger.length() < 256) {
             EEPROM.writeString(EEPROM_ADDRESS_TRIGGER, configTrigger);
             EEPROM.commit();
           }
         }
       } else {
         if (configTrigger.length() > 1) {
           configTrigger = "";
           EEPROM.writeString(EEPROM_ADDRESS_TRIGGER, configTrigger);
           EEPROM.commit();
         }
       }
     }

   } else {
     //        Serial.print("Error code: ");
     //        Serial.println(httpResponseCode);
     time_to_sleep_mode = TIME_TO_SLEEP;
   }
   // Free resources
   http.end();
   delete client;
 }
 /*
 Method to print the reason by which ESP32
 has been awaken from sleep
 */
 void print_wakeup_reason() {
   esp_sleep_wakeup_cause_t wakeup_reason;

   wakeup_reason = esp_sleep_get_wakeup_cause();

   switch (wakeup_reason) {
   case ESP_SLEEP_WAKEUP_EXT0:
     Serial.println("Wakeup caused by external signal using RTC_IO");
     break;
   case ESP_SLEEP_WAKEUP_EXT1:
     Serial.println("Wakeup caused by external signal using RTC_CNTL");
     break;
   case ESP_SLEEP_WAKEUP_TIMER:
     Serial.println("Wakeup caused by timer");
     break;
   case ESP_SLEEP_WAKEUP_TOUCHPAD:
     Serial.println("Wakeup caused by touchpad");
     break;
   case ESP_SLEEP_WAKEUP_ULP:
     Serial.println("Wakeup caused by ULP program");
     break;
   default:
     Serial.printf("Wakeup was not caused by deep sleep: %d\n", wakeup_reason);
     break;
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

 void loop() {
   //This is not going to be called
 }
