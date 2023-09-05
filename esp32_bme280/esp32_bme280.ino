 #include <WiFi.h>

 #include <EEPROM.h>

 #include <WiFiClientSecure.h>

 #include <HTTPClient.h>

 #include <ArduinoJson.h>

#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>


 #define uS_TO_S_FACTOR 1000000ULL /* Conversion factor for micro seconds to seconds */
 #define EEPROM_SIZE 256
 #define TIME_TO_SLEEP 30


 RTC_DATA_ATTR int bootCount = 0;


 String deviceID = "";
 String serialNumber = "";
 String serverName = "https://telua.co/service/v1/esp32/update-sensor";

 int EEPROM_ADDRESS_SSID = 0;
 int EEPROM_ADDRESS_PASS = 32;
 int EEPROM_ADDRESS_TIME_TO_SLEEP = 64;
 int EEPROM_ADDRESS_DEVICE_ID = 128;
 int EEPROM_ADDRESS_SERIAL_NUMBER = 192;

 bool hasRouter = false;
 bool hasSensor = false;
 int time_to_sleep_mode = TIME_TO_SLEEP;
 
Adafruit_BME280 bme;

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
   Serial.println("Telua bme test");
   if (!bme.begin(0x76)) {
     Serial.println("Couldn't find bme");
   } else {
     hasSensor = true;
     Serial.println("Found bme sensor");
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

 void sendReport() {
   if (WiFi.status() != WL_CONNECTED) {
     return;
   }
   //https://lastminuteengineers.com/bme280-arduino-tutorial/
   String temperature = "0";
   String relative_humidity = "0";
   if (hasSensor == true) {
     temperature = String(bme.readTemperature(), 2);
     relative_humidity = String(bme.readHumidity(), 2);
   }
   
   WiFiClientSecure * client = new WiFiClientSecure;
   if (!client) {
     return;
   }

   client -> setInsecure();
   HTTPClient http;
   String serverPath = serverName + "?sensorName=BME280&temperature=" + temperature + "&humidity=" + relative_humidity + "&deviceID=" + deviceID + "&serialNumber=" + serialNumber;

   http.begin( *client, serverPath.c_str());

   // Send HTTP GET request
   int httpResponseCode = http.GET();

   if (httpResponseCode == 200) {
     //        Serial.print("HTTP Response code: ");
     //        Serial.println(httpResponseCode);
     String payload = http.getString();
     //      Serial.println(payload);

     //https://arduinojson.org/v6/doc/upgrade/
     DynamicJsonDocument doc(2048);

     DeserializationError error = deserializeJson(doc, payload);
     if (error) {
       Serial.println("deserializeJson() failed");

     } else {

       int intervalTime = doc["intervalTime"];
       Serial.print("deserializeJson intervalTime=");
       Serial.println(intervalTime);
       if (intervalTime >= 30) {

         if (time_to_sleep_mode != intervalTime && intervalTime <<= 1800) {
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
