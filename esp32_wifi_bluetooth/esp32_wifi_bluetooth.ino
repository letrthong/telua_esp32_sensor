#include <WiFi.h>
#include "Adafruit_SHT4x.h"

#include <HTTPClient.h>

const char* ssid = "hcmus";   /*SSID of network to connect*/
const char* password = "fetelxxx";  /*password for SSID*/

String serverName = "http://34.111.197.130:80/service/v1/esp32/update-sensor";

unsigned long previousMillis = 0;
unsigned long interval = 30000;


void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());
}
  

 
void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  initWiFi();
  Serial.print("RSSI (WiFi strength): ");
  Serial.println(WiFi.RSSI());
   
}

void loop() {
   Serial.println("Hello from Telua ESP-WROOM-32");
   delay(1000);
    unsigned long currentMillis = millis();
  /*if condition to check wifi reconnection*/
  if ((WiFi.status() != WL_CONNECTED) && (currentMillis - previousMillis >=interval)) {
    Serial.print(millis());
    Serial.println("Reconnecting to WiFi...");
    WiFi.disconnect();
    WiFi.reconnect();
    previousMillis = currentMillis;
  }else if ((currentMillis - previousMillis >=interval)){
      HTTPClient http; 
      String serverPath = serverName + "?temperature=24.37";
      http.begin(serverPath.c_str());
      
      // Send HTTP GET request
      int httpResponseCode = http.GET();
      
      if (httpResponseCode>0) {
        Serial.print("HTTP Response code: ");
        Serial.println(httpResponseCode);
        String payload = http.getString();
        Serial.println(payload);
      }
      else {
        Serial.print("Error code: ");
        Serial.println(httpResponseCode);
      }
      // Free resources
      http.end();
      previousMillis = currentMillis;
  }
}
