#include <WiFi.h>
#include <EEPROM.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "Adafruit_SHT4x.h"


#define uS_TO_S_FACTOR 1000000ULL /* Conversion factor for micro seconds to seconds */
#define EEPROM_SIZE 512
#define TIME_TO_SLEEP 30

RTC_DATA_ATTR int bootCount = 0;
RTC_DATA_ATTR bool isCorrectPassword = false;
String deviceID = "";
String serialNumber = "";
String configTrigger = "";
String select_html = "";
String remote_ssid = "";
String remote_pass = "";

String serverName = "https://telua.co/service/v1/esp32/update-sensor";
String error_url = "https://telua.co/service/v1/esp32/error-sensor";
String trigger_url = "https://telua.co/service/v1/esp32/trigger-sensor";

int EEPROM_ADDRESS_SSID = 0;
int EEPROM_ADDRESS_PASS = 32;
int EEPROM_ADDRESS_REMOTE_SSID = 48;
int EEPROM_ADDRESS_REOMVE_PASS = 64;
int EEPROM_ADDRESS_TIME_TO_SLEEP = 96; 
int EEPROM_ADDRESS_DEVICE_ID = 128;
int EEPROM_ADDRESS_SERIAL_NUMBER = 192;
int EEPROM_ADDRESS_TRIGGER = 256;

bool hasRouter = false;
RTC_DATA_ATTR int g_encryption_Type = WIFI_AUTH_OPEN;

bool hasRemoteRouter = false;
RTC_DATA_ATTR int g_remtoe_encryption_Type = WIFI_AUTH_OPEN;

bool hasSensor = false;
bool hasError = true;
RTC_DATA_ATTR int retryTimeout = 0;

int time_to_sleep_mode = TIME_TO_SLEEP;

Adafruit_SHT4x sht4 = Adafruit_SHT4x();

const char * ssid = "Telua_Sht40_";
const char * ssid_gpio = "Telua_Sht40_Gpio_"; 

const char * password = "12345678";
String g_ssid = "";
unsigned long previousMillis = 0;
unsigned long interval = 30000;

unsigned long previousMillisLocalWeb = 0;
unsigned long intervalLocalWeb = 30000;




// the LED is connected to GPIO 5
bool hasGPIo = true;
const int ledRelay01 = 17 ; 
const int ledRelay02 =  5; 
const int ledAlarm =  19; 
const int ledFloatSwitch =  4; 

const int btnTop = 18;
const int btnBot = 16;

void intGpio(){
    pinMode(ledRelay01, OUTPUT);
    pinMode(ledRelay02, OUTPUT);
    pinMode(ledAlarm, OUTPUT);
//  pinMode(ledFloatSwitch, OUTPUT);  

//  pinMode(btnTop, INPUT); 
//  pinMode(btnBot, INPUT);  
   turnOffAll();
}

void turnOffAll(){
   digitalWrite(ledRelay01, LOW);
   digitalWrite(ledRelay02, LOW);
   digitalWrite(ledAlarm, LOW);
   
//   digitalWrite(ledFloatSwitch, LOW);
//
//   
//   int buttonState = digitalRead(btnTop);
//    if (buttonState == HIGH) {
//        digitalWrite(ledRelay01, HIGH);
//    }
//
//     buttonState = digitalRead(btnBot);
//    if (buttonState == HIGH) {
//        digitalWrite(ledRelay02, HIGH);
//    }
}

bool turnOnRelay(String action){
   bool retCode  = false;
   
   if( action =="b1On"){
       digitalWrite(ledRelay01, HIGH);
        retCode = true;
   }else  if( action =="b2On"){
      digitalWrite(ledRelay02, HIGH);
       retCode = true;
   } else  if( action == "alOn"){
       digitalWrite(ledAlarm, HIGH);
       retCode = true;
   } 

   return retCode;
}

bool turnOffRelay(String action){
   bool retCode  = false;
   if( action =="b1Off"){
       digitalWrite(ledRelay01, LOW);
   }else  if( action =="b2Off"){
      digitalWrite(ledRelay02, LOW);
   } else  if( action =="alOff"){
       digitalWrite(ledAlarm, LOW);
   } 
   return retCode;
}

bool hasTrigger(){
    bool retCode  = false;
   if (configTrigger.length() > 2 /*&& hasSensor == true*/) {
      if(hasGPIo == true){
          retCode = true;
      }
   }
   return retCode;
}


void startSleepMode() {
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

void startLocalWeb() {
  WiFi.mode(WIFI_AP_STA);
  WiFiServer server(80);
  Serial.print("Setting AP (Access Point)…");
  // Remove the password parameter, if you want the AP (Access Point) to be open
  IPAddress local_ip(192, 168, 0, 1);
  IPAddress gateway(192, 168, 0, 1);
  IPAddress subnet(255, 255, 255, 0);
  int randNumber = random(300);
  WiFi.softAPConfig(local_ip, gateway, subnet);
  //    WiFi.softAP(ssid + String(randNumber), password);
  if(hasGPIo == false){
     WiFi.softAP(ssid + serialNumber, password); 
  }else{
     WiFi.softAP(ssid_gpio + serialNumber, password);
  }
 

  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);

  server.begin();
  String header;
  bool hasConnection = false;
  int count = 0;
  int countWaitRequest = 0;
  
  unsigned long currentMillisLocalWeb = millis();
  previousMillisLocalWeb = currentMillisLocalWeb;
  while (1) {
    WiFiClient client = server.available();
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= interval) {
      previousMillis = currentMillis;
      Serial.println("waiting connection");
      count = count + 1;
      if (hasConnection == true) {
        hasConnection = false;
        count = 9;
      }
    }

    if (count >= 10) {
      server.close();
      WiFi.disconnect();
      delay(100);
      time_to_sleep_mode = 30;
      if(hasTrigger() == true){
        return;
      }
      startSleepMode();
      return;
    }

    if (client) { // If a new client connects,
      Serial.println("New Client."); // print a message out in the serial port
      String currentLine = ""; // make a String to hold incoming data from the client
      bool hasWrongFormat = false;
      String privateIpv4 = "";
      while (client.connected()) { // loop while the client's connected
        if (client.available()) { // if there's bytes to read from the client,
           unsigned long currentMillisLocalWeb = millis();
           previousMillisLocalWeb = currentMillisLocalWeb;
           countWaitRequest = 0; 
          char c = client.read(); // read a byte, then
          //            Serial.write(c);                    // print it out the serial monitor
          header += c;
          if (c == '\n') { // if the byte is a newline character
            // if the current line is blank, you got two newline characters in a row.
            // that's the end of the client HTTP request, so send a response:
            if (currentLine.length() == 0) {
              // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
              // and a content-type so the client knows what's coming, then a blank line:
              client.println("HTTP/1.1 200 OK");
              client.println("Content-type:text/html");
              client.println("Connection: close");
              client.println();

              int index = header.indexOf("/router_info?ssid=");
              if (index >= 0) {
                String parameters = header.substring(index + 18);
                index = parameters.indexOf("HTTP");

                if (index > 0) {
                  String info = parameters.substring(0, index);
                  Serial.println("/router_info");
                  Serial.println(info);
                  index = info.indexOf("&password=");

                  String ssid = info.substring(0, index);
                  ssid.replace("+", " ");

                  String passowrd = info.substring(index + 10);
                  passowrd.replace(" ", "");

                  Serial.print("ssid=[");
                  Serial.print(ssid);
                  Serial.print("]");

                  Serial.print("passowrd=[");
                  Serial.print(passowrd);
                  Serial.print("]");

                  if (ssid.length() > 0) {
                    if (passowrd.length() == 0) {
                      Serial.println("WIFI_AUTH_OPEN");
                      WiFi.begin(ssid);
                      passowrd = "12345678";
                    } else {
                      WiFi.begin(ssid, passowrd);
                    }

                    Serial.print("Connecting to WiFi ..");
                    int count = 0;
                    while (WiFi.status() != WL_CONNECTED) {
                      Serial.print('.');
                      delay(500);
                      // 15 seconds
                      count = count + 1;
                      if (count > 30) {
                        break;
                      }
                    }

                    if (WiFi.status() == WL_CONNECTED) {
                      Serial.println(WiFi.localIP());
                      privateIpv4 = WiFi.localIP().toString().c_str();
                      hasConnection = true;

                      EEPROM.writeString(EEPROM_ADDRESS_SSID, ssid);
                      EEPROM.commit();

                      EEPROM.writeString(EEPROM_ADDRESS_PASS, passowrd);
                      EEPROM.commit();
                    } else {
                      hasWrongFormat = true;
                    }
                  } else {
                    hasWrongFormat = true;
                  }

                }
              }

              // Display the HTML web page
              client.println("<!DOCTYPE html><html>");
              client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
              client.println("<link rel=\"icon\" href=\"data:,\">");
              // Web Page Heading 
              client.println("<body><h4>Telua Nen Tang Cho IoT- Telua IoT platform - 20-08-2023 </h4>");
              if (serialNumber.length() > 0) {
                client.println("<p>Serial Number=" + serialNumber + "</p>");
              }

              if (g_ssid.length() > 0) {
                client.println("<p>Ten mang-Network name=" + g_ssid + "</p>");
              }

              client.println("<form action=\"/router_info\"  method=\"get\">");
              client.println("<label style=\"color:blue;\">Chon SSID cua Wi-Fi - Select SSID of Wi-Fi</label><br>");
              //client.println("<input type=\"text\" style=\"height:25px;\"  id=\"ssid\" name=\"ssid\" value=\"\"><br>");
              client.println(select_html);
              client.println("<label>Mat Khau cua Wi-Fi - Password of Wi-Fi</label><br>");
              client.println("<input type=\"text\" style=\"height:25px;\" id=\"password\" name=\"password\" value=\"\"><br>");
              client.println("<input type=\"submit\" style=\"margin-top:20px; height:40px;\"  value=\"Xac Nhan - Submit\">");
              client.println("</form>");
              if (hasWrongFormat == true) {
                client.println("<p>Xin kiem tra lai SSID va Mat Khau cua Wi-Fi</p>");
                client.println("<p>Please recheck SSID and password of Wi-Fi</p>");
              } else {
                if (privateIpv4.length() > 0) {
                  client.println("<p  style=\"color:red;\"> Thiet bi co the ket noi toi Internet voi IPv4=" + privateIpv4 + "</p>");
                  client.println("<p  style=\"color:red;\"> The device can connect the internet with IPv4=" + privateIpv4 + "</p>");

                  client.println("<h4> </h4>");
                  client.println("<a href=\"https://telua.co/aiot\"  >https://telua.co/aiot</a>");
                }
              }

              client.println("</body></html>");

              // The HTTP response ends with another blank line
              client.println();
              // Break out of the while loop
              break;
            } else { // if you got a newline, then clear currentLine
              currentLine = "";
            }
          } else if (c != '\r') { // if you got anything else but a carriage return character,
            currentLine += c; // add it to the end of the currentLine
          }
        } else{
          unsigned long currentMillisLocalWeb = millis();
          if (currentMillisLocalWeb - previousMillisLocalWeb >= intervalLocalWeb) {
            previousMillisLocalWeb = currentMillisLocalWeb;
            Serial.println("waiting httpRequest");
            countWaitRequest = countWaitRequest + 1;
            if(countWaitRequest >=2){
              countWaitRequest = 0;
              break;
            }
          }
        }
      }
      // Clear the header variable
      header = "";
      // Close the connection
      client.stop();
    }
  }
}

void startSmartConfig() {
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
}

void initWiFi() {
  WiFi.mode(WIFI_STA);

  String current_ssid = EEPROM.readString(EEPROM_ADDRESS_SSID);
  String current_pass = EEPROM.readString(EEPROM_ADDRESS_PASS);

  unsigned int Length_of_ssid = current_ssid.length();
  g_ssid = current_ssid;
  hasRouter = false;
  if (isCorrectPassword == false) {
    for (int y = 0; y < 3; y++) {
      int n = WiFi.scanNetworks();
      Serial.println("Scan done");
      if (n == 0) {
        Serial.println("no networks found");
      } else {
        Serial.print(n);
        Serial.println(" networks found");
        select_html = " <select  id=\"ssid\"  style=\"height:30px; width:120px;\"   name=\"ssid\">";
        for (int i = 0; i < n; ++i) {
          String SSID = WiFi.SSID(i);
          Serial.print("scanNetworks SSID=");
          Serial.println(SSID);
          if (i < 10) {
            select_html = select_html + "<option value=\"" + SSID + "\">" + SSID + "</option>";
          }

          if (Length_of_ssid > 0) {
            if (current_ssid.equals(SSID)) {
              hasRouter = true;
              g_encryption_Type = WiFi.encryptionType(i);
              //break;
            }

            if (remote_ssid.equals(SSID)) {
              hasRemoteRouter = true;
              g_remtoe_encryption_Type = WiFi.encryptionType(i);
              //break;
            }

          }
        }

        if (n < 1) {
          select_html = select_html + "<option value=\" \"> </option>";
        }
        select_html = select_html + " </select> <br>";
      }
      WiFi.scanDelete();

      if (hasRouter == true) {
        break;
      }
      delay(500);
    }
  }

  if (hasRouter == true || isCorrectPassword == true) {
    if (g_encryption_Type == WIFI_AUTH_OPEN) {
      Serial.println("WIFI_AUTH_OPEN");
      WiFi.begin(current_ssid);
    } else {
      WiFi.begin(current_ssid, current_pass);
    }

    Serial.print(current_ssid);
    Serial.print(" Connecting to WiFi ..");
    int count = 0;
    int retryTime = 30;
    if (isCorrectPassword == true) {
      retryTime = 60;
    }
    while (WiFi.status() != WL_CONNECTED) {
      Serial.print('.');
      delay(500);
      // 15 seconds
      count = count + 1;
      if (count > retryTime) {
        break;
      }
    }

    if (WiFi.status() == WL_CONNECTED) {
      isCorrectPassword = true;
    }
  }

  if (isCorrectPassword == false) {
    // Retry again
      Serial.println(" Retry with the remote router");
    bool isConnecting = false;
    if (hasRemoteRouter == true) {
      WiFi.disconnect();
      delay(500);
      if (g_remtoe_encryption_Type == WIFI_AUTH_OPEN) {
        Serial.println("WIFI_AUTH_OPEN");
        WiFi.begin(remote_ssid);
        isConnecting = true;
      } else {
        if(remote_pass.length() > 1){
           WiFi.begin(remote_ssid, remote_pass);
           isConnecting = true;
        }  else {
          EEPROM.writeString(EEPROM_ADDRESS_REMOTE_SSID, "");
          EEPROM.commit();
          ESP.restart();
        }
      }
      Serial.print("Connecting to Remote WiFi ..");
      int count = 0;
      int retryTime = 30;
      while (WiFi.status() != WL_CONNECTED &&  (isConnecting == true)) {
        Serial.print('.');
        delay(500);
        // 15 seconds
        count = count + 1;
        if (count > retryTime) {
          break;
        }
      }

      if (WiFi.status() == WL_CONNECTED) {
        Serial.println("Remote WL_CONNECTED");
        if (current_ssid != remote_ssid) {
          EEPROM.writeString(EEPROM_ADDRESS_SSID, remote_ssid);
          EEPROM.commit();

          EEPROM.writeString(EEPROM_ADDRESS_PASS, remote_pass);
          EEPROM.commit();
          ESP.restart();
        }
      } else {
        if(isConnecting == true){
           EEPROM.writeString(EEPROM_ADDRESS_REMOTE_SSID, "");
           EEPROM.commit();
           ESP.restart();
        }
      }
    }
  }

  Serial.println(WiFi.localIP());
  if (WiFi.status() != WL_CONNECTED && isCorrectPassword == false) {
    startLocalWeb();
  }
//    else{
//        startLocalWeb();
//    }

}

void turnOffWiFi() {
  WiFi.disconnect();
}

void initSht4x() {
  hasSensor = false;
  Serial.println("Telua SHT4x test");
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

  configTrigger = EEPROM.readString(EEPROM_ADDRESS_TRIGGER);
  Serial.print("initEEPROM configTrigger=");
  Serial.println(configTrigger);

  remote_ssid = EEPROM.readString(EEPROM_ADDRESS_REMOTE_SSID);
  Serial.print("initEEPROM remote_ssid=");
  Serial.println(remote_ssid);

  remote_pass = EEPROM.readString(EEPROM_ADDRESS_REOMVE_PASS);
  Serial.print("initEEPROM remote_pass=");
  Serial.println(remote_pass);
}

bool sendReport(bool hasReport) {
  bool ret = false;
  String temperature = "0";
  String relative_humidity = "0";
  float fHumidity = 0.0;
  float fTemperature = 0.0;
  if (hasSensor == true) {
    for (int i = 0; i < 3; i++) {
      sensors_event_t humidity, temp;
      sht4.getEvent( & humidity, & temp);

      //      Serial.print("Temperature: ");
      //      Serial.print(temp.temperature);
      //      Serial.println(" degrees C");
      //      Serial.print("Humidity: "); 
      //      Serial.print(humidity.relative_humidity);
      //      Serial.println("% rH");
      fHumidity = humidity.relative_humidity;
      fTemperature = temp.temperature;
      temperature = String(fTemperature, 2);
      relative_humidity = String(fHumidity, 2);
      if (fHumidity > 0) {
        hasError = false;
        break;
      }
      delay(500);
    }
  }

   String strTriggerParameter = "";
  //process trigger
  if (configTrigger.length() > 1 /*&& hasSensor == true*/) {
    StaticJsonDocument < 2048 > docTrigger;

    // parse a JSON array
    DeserializationError errorTrigger = deserializeJson(docTrigger, configTrigger);

    if (errorTrigger) {
      Serial.println("deserializeJson() failed");
      strTriggerParameter = "ConfigError";
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
          currentValue = fTemperature;
           if(hasSensor == false || hasError == true  ){
             continue;
          }
        } else if (property == "tem") {
          currentValue = fTemperature;
          if(hasSensor == false || hasError == true ){
             continue;
          }
        } else if (property == "humidity") {
          currentValue = fHumidity;
           if(hasSensor == false || hasError == true ){
             continue;
          }
        }  else if (property == "hum") {
          currentValue = fHumidity;
          if(hasSensor == false || hasError == true ){
             continue;
          }
        } else if (property == "err"){
          if (hasSensor   == false || hasError == true ){
            hasTrigger = true;
          }
        }
        
        if (property != "error"){
            if (opera == "=") {
                if (currentValue == value) {
                  hasTrigger = true;
                }
              } else if (opera == "<") {
                if (currentValue < value) {
                  hasTrigger = true;
                    }
            } else if (opera == ">") {
              if (currentValue > value) {
                hasTrigger = true;
              }
            } else if (opera == ">=") {
              if (currentValue >= value) {
                hasTrigger = true;
              }
            } else if (opera == "<=") {
              if (currentValue <= value) {
                hasTrigger = true;
              }
            } else if (opera == "!=") {
              if (currentValue != value) {
                hasTrigger = true;
              }
           }
        }
        // -- start hasTrigger----------------
        if (hasTrigger == true) {
          strTriggerParameter = strTriggerParameter + action + "-";
           if(hasGPIo == true){
               int index = action.indexOf("On");
                if (index >= 0) {
                   bool result = turnOnRelay(action);
                   if(result == true){
                        ret = true;
                   }
                } else {
                  index = action.indexOf("Off");
                  if (index >= 0) {
                      bool result = turnOffRelay(action);
                     if(result == true){
                          ret = true;
                     }
                  }
                }
            }
        }
        //  -- End hasTrigger----------------
      }
    }
  }
  
   if(hasReport == false){
      return ret;
   }
   
  if (WiFi.status() != WL_CONNECTED) {
    time_to_sleep_mode = 60;
    Serial.println("sendReport WiFi.status() != WL_CONNECTED");
    if(hasTrigger() == true){
      return ret;
    }
    return false;
  }

  String localIP = WiFi.localIP().toString();
  if (localIP == "0.0.0.0") {
    time_to_sleep_mode = 60;
    Serial.println("sendReport  localIP= 0.0.0.0");
    if(hasTrigger() == true){
      return ret;
    }
    return false;
  }

  WiFiClientSecure * client = new WiFiClientSecure;
  if (!client) {
    return false;
  }

  
  client -> setInsecure();
  HTTPClient http;
  String serverPath = serverName + "?sensorName=SHT40&temperature=" + temperature + "&humidity=" + relative_humidity + "&deviceID=" + deviceID + "&serialNumber=" + serialNumber;

  if(hasGPIo == true){
    serverPath = serverName + "?sensorName=SHT40_Controller&temperature=" + temperature + "&humidity=" + relative_humidity + "&deviceID=" + deviceID + "&serialNumber=" + serialNumber;
  }
  
  if (strTriggerParameter.length() > 0) {
    serverPath = trigger_url + "?deviceID=" + deviceID + "&temperature=" + temperature + "&humidity=" + relative_humidity + +"&trigger=" + strTriggerParameter;
  }

  if (hasError == true) {
      serverPath = error_url + "?sensorName=SHT40&deviceID=" + deviceID + "&serialNumber=" + serialNumber;
      if(hasGPIo == true){
          serverPath = error_url + "?sensorName=SHT40_Controller&deviceID=" + deviceID + "&serialNumber=" + serialNumber;
     }
  }
  Serial.println(serverPath);

  http.setTimeout(60000);
  http.begin( * client, serverPath.c_str());

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

      // Router info
      bool hasSSID = doc.containsKey("ssid");
      bool hasPassword = doc.containsKey("password");
      if (hasSSID == true && hasPassword == true) {
        String ssid = doc["ssid"];
        String password = doc["password"];
        Serial.print("deserializeJson ssid=");
        Serial.println(ssid);

        Serial.print("deserializeJson password=");
        Serial.println(password);
        if (ssid.length() > 0 && remote_ssid != ssid) {
          remote_ssid = ssid;
          EEPROM.writeString(EEPROM_ADDRESS_REMOTE_SSID, remote_ssid);
          EEPROM.commit();
        } else {
          Serial.println("remote_ssid is the same");
        }

        if (remote_pass != password) {
          remote_pass = password;
          EEPROM.writeString(EEPROM_ADDRESS_REOMVE_PASS, remote_pass);
          EEPROM.commit();
        }
      }

      // Device ID
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
        Serial.println("strTrigger.length()=" + String(strTrigger.length()));
        if (configTrigger != strTrigger) {
         
           if(hasGPIo == true){
            if (strTrigger.length() < 256){
               configTrigger = strTrigger;
                turnOffAll();
            } 
          }else{
             configTrigger = strTrigger; 
          }
          
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
    retryTimeout = 0;
  } else {
     Serial.print("Error code: ");
     Serial.println(httpResponseCode);
      time_to_sleep_mode = TIME_TO_SLEEP;
      retryTimeout = retryTimeout + 1;
      //Timeout - https://github.com/esp8266/Arduino/issues/5137
      if(httpResponseCode == -11){
        http.end();
        delete client;
        delay(3000);
        Serial.println("sendReport retryTimeout=" + String(retryTimeout));
         if(hasGPIo == true){
              if(retryTimeout > 3){
                 ESP.restart();
              }else{
                return ret;
            } 
         }else{
              ESP.restart();
         }
       
      }
  }
  // Free resources
  http.end();
  delete client;

  return ret;
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
  if (bootCount >= 60) {
    bootCount = 0;
    ESP.restart();
  }
  Serial.println("Ver:8/Aug/2023");
  //Increment boot number and print it every reboot
  ++bootCount;
  Serial.println("Boot number: " + String(bootCount));

  if(hasGPIo == true){
    intGpio();
  }
  initEEPROM();
  initWiFi();

  initSht4x();

  
  if(hasGPIo == false){
    sendReport(true); 
  }else{
      for(int i = 0; i< 15; i++){
        bool ret = sendReport(true);
        if(ret == true){
           delay(1000);
           for(int i = 0; i < time_to_sleep_mode; i++){
              if (sendReport(false) == false){
                  break;
              }
             
              Serial.println("sendReport count=" + String(i));
              delay(1000);
          }
        }else{
          break;
        }
      }
  }
 
  
  turnOffWiFi();

  //Print the wakeup reason for ESP32
  print_wakeup_reason();
  startSleepMode();

}

void loop() {
  //This is not going to be called
}
