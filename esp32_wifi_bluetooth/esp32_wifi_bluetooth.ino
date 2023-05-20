#include <WiFi.h>

#include <EEPROM.h>

#include <HTTPClient.h>

#include <ArduinoJson.h>

#include "Adafruit_SHT4x.h"


#define EEPROM_SIZE 64

String default_ssid = "telua"; /*SSID of network to connect*/
String default_password = "12345678"; /*password for SSID*/

const char * deviceID = "12334332343443ADVED";

String serverName = "http://34.111.197.130:80/service/v1/esp32/update-sensor";
String url_ssid = "http://34.111.197.130:80/service/v1/esp32/ssid-info";

unsigned long previousMillis = 0;
unsigned long interval = 30000;

int EEPROM_ADDRESS_SSID = 0;
int EEPROM_ADDRESS_PASS = 32;

bool hasRouter = false;
bool hasDefaultRouter = false;
bool isConnectingDefaultRouter = false;
int total_rettry = 0;

Adafruit_SHT4x sht4 = Adafruit_SHT4x();

bool hasSensor = false;

void initWiFi() {
  WiFi.mode(WIFI_STA);

  String current_ssid = EEPROM.readString(EEPROM_ADDRESS_SSID);
  String current_pass = EEPROM.readString(EEPROM_ADDRESS_PASS);
  unsigned int lastStringLength = current_ssid.length();

  hasRouter = false;
  hasDefaultRouter = false;
  isConnectingDefaultRouter = false;

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
        }
      }

      if (default_ssid.equals(SSID)) {
        hasDefaultRouter = true;
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

  if (WiFi.status() != WL_CONNECTED) {
    if (hasDefaultRouter == true) {
      Serial.print("Connecting to default WiFi  ..");
      WiFi.disconnect();
      WiFi.begin(default_ssid, default_password);
      delay(1000);
      WiFi.reconnect();
      isConnectingDefaultRouter = true;
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
  }

  Serial.println(WiFi.localIP());
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

  //   String myStr = EEPROM.readString(EEPROM_ADDRESS_SSID);
  //   unsigned int lastStringLength = myStr.length();
  //   if(lastStringLength <1){
  //      myStr = "hcmus";
  //      EEPROM.writeString(EEPROM_ADDRESS_SSID, myStr);
  //      EEPROM.commit();
  //      Serial.println("initEEPROM writeString");
  //   }else{
  //     Serial.print("initEEPROM readString myStr=");
  //     Serial.println(myStr);
  //   }
  //
  //   myStr = EEPROM.readString(EEPROM_ADDRESS_PASS);
  //   lastStringLength = myStr.length();
  //   if(lastStringLength <1){
  //      myStr = "fetelxxx";
  //      EEPROM.writeString(EEPROM_ADDRESS_PASS, myStr);
  //      EEPROM.commit();
  //      Serial.println("initEEPROM writeString");
  //   }else{
  //     Serial.print("initEEPROM readString myStr=");
  //     Serial.println(myStr);
  //   }

  // myStr = EEPROM.readString(EEPROM_ADDRESS_PASS);
}

void setup() {
  Serial.begin(115200);

  initEEPROM();

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
  if ((WiFi.status() != WL_CONNECTED) && (currentMillis - previousMillis >= interval)) {
    total_rettry = total_rettry + 1;
    if (total_rettry > 10) {
      ESP.restart();
    }

    Serial.print(millis());
    Serial.println("  Reconnecting to WiFi...");
    WiFi.disconnect();
    delay(1000);
    WiFi.reconnect();
    previousMillis = currentMillis;
  } else if ((currentMillis - previousMillis >= interval)) {
    total_rettry = 0;
    if (isConnectingDefaultRouter == true) {
      String serverPath = url_ssid + "?deviceID=" + deviceID;
      HTTPClient http;
      http.begin(serverPath.c_str());
      int httpResponseCode = http.GET();
      if (httpResponseCode > 0) {
        if (httpResponseCode == 200) {
          String payload = http.getString();
          DynamicJsonDocument doc(1024);

          DeserializationError error = deserializeJson(doc, payload);
          if (error) {
            Serial.println("deserializeJson() failed");

          } else {
            Serial.println("deserializeJson");
            String ssid = doc["ssid"];
            String pass = doc["pass"];

            if (ssid.length() > 1 && pass.length() >= 8) {
              EEPROM.writeString(EEPROM_ADDRESS_SSID, ssid);
              EEPROM.commit();

              EEPROM.writeString(EEPROM_ADDRESS_PASS, pass);
              EEPROM.commit();

              http.end();
              delay(1000);
              ESP.restart();
            }
          }
        }

      }

      http.end();

    } else {
      String temperature = "0";
      String relative_humidity ="0";
      if(hasSensor == true){
         sensors_event_t humidity, temp;
        
        uint32_t timestamp = millis();
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

      if (httpResponseCode > 0) {
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
              interval = intervalTime;
          }
        }

      } else {
        //        Serial.print("Error code: ");
        //        Serial.println(httpResponseCode);
      }
      // Free resources
      http.end();
      previousMillis = currentMillis;

    }
  }
}
