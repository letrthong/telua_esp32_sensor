#include <WiFi.h>
#include "time.h"
#include <EEPROM.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
 
#include <Adafruit_MCP4725.h>
Adafruit_MCP4725 MCP4725;
// http://www.esp32learning.com/code/esp32-and-mcp4725-digital-to-analog-converter-example.php

#define uS_TO_S_FACTOR 1000000ULL /* Conversion factor for micro seconds to seconds */
#define EEPROM_SIZE 512
#define TIME_TO_SLEEP 30

RTC_DATA_ATTR int bootCount = 0;
RTC_DATA_ATTR bool isCorrectPassword = false;
String deviceID = "";
String serialNumber = "";
String configTrigger = "";
String configScheduler = "";
String select_html = "";
String remote_ssid = "";
String remote_pass = "";
String currentPwm = "";
bool gHasSpeed = false;

String serverConfig = "https://telua.co/service/v1/esp32/pmw/config";
String serverError = "https://telua.co/service/v1/esp32/pmw/status";
String serverOffset = "https://telua.co/service/v1/esp32/gmtOffset"; 
String releaseDate = "29-Aug-2024";
int gUptime = 0;
int gUptimeCounter = 0;
int gPreUptime = 0;
int startEpchoTime = 0;

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
int g_count = 60;
int time_to_sleep_mode = TIME_TO_SLEEP;
 
const char * ssid = "Telua_PWM_";
 
const char * password = "12345678";
String g_ssid = "";
unsigned long previousMillis = 0;
unsigned long interval = 60000;

unsigned long previousMillisLocalWeb = 0;
unsigned long intervalLocalWeb = 60000;

const char* ntpServer = "pool.ntp.org";
// 25200 = 7*60*60  +7
long gmtOffset_sec = 25200;
const int daylightOffset_sec = 0;
String currentPercent  = "";
  
bool hasSensorError = false;
 //3.3 is your supply voltage
const float max_Voltage = 3.3;

int gCount = 0;

void initSensor(){
  // MCP4725_I2CADDR_DEFAULT = 0x62
    bool ret = MCP4725.begin(MCP4725_I2CADDR_DEFAULT); // The I2C Address of my module  
    if(ret == false){
      Serial.println("  Can not detect MCP4725 ");
      hasSensorError = true;
    } else {
        setSpeed("0", false);
        //createTrianglewave();
    }
}


void createTrianglewave(){
    uint32_t counter;
    // Run through the full 12-bit scale for a triangle wave
    for (counter = 0; counter < 4095/max_Voltage; counter++)
    {
        Serial.print("createTrianglewave counter=");
        Serial.println(counter);
        MCP4725.setVoltage(counter, false);
        delay(5); 
    }

    for (counter = 4095/max_Voltage; counter > 0; counter--)
    {
      Serial.print("createTrianglewave counter=");
      Serial.println(counter);
      MCP4725.setVoltage(counter, false);
      delay(5); 
    }
}
 
void setSpeed(String percent,  bool hasRestat){

    if(gCount > 100){
      currentPercent  ="";
    }

    currentPwm = String(percent);

    if(currentPercent == percent){
        Serial.print("setSpeedsame percent=");
        Serial.println(percent);
        gCount = gCount +1;
        return;
    }
    gCount = 0;
    

    int intPercent = percent.toInt();

   
    bool writeEEPROM  = false;
    uint32_t MCP4725_value =  ( (4096/max_Voltage)/100 ) * intPercent;
    bool ret = MCP4725.setVoltage(MCP4725_value, writeEEPROM);

    float  MCP4725_reading = (max_Voltage/4096.0) * MCP4725_value; 


    Serial.println("");
    Serial.print("setSpeed Expected Voltage: ");
    Serial.println(MCP4725_reading, max_Voltage);

    if(ret == false){
        Serial.print("\t Can not write MCP4725");
        currentPercent = "";
        hasSensorError = true;

        if(hasRestat == true){
            sendError(true);

            delay(10000); 
            ESP.restart();
        }
       
    }  

    if(currentPercent != percent){
        currentPercent = percent;
        if(hasRestat == true){
            sendError(false);
        }
    }
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
  WiFi.softAP(ssid + serialNumber, password); 
 

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

 // String current_ssid = EEPROM.readString(EEPROM_ADDRESS_SSID);
// String current_pass = EEPROM.readString(EEPROM_ADDRESS_PASS);

 String current_ssid = "telua";
 String current_pass = "13572468";

  unsigned int Length_of_ssid = current_ssid.length();
  g_ssid = current_ssid;
  hasRouter = false;
   bool hasNetworks = false;
  if (isCorrectPassword == false) {
    for (int y = 0; y < 3; y++) {
      int n = WiFi.scanNetworks();
      Serial.println("Scan done");
      if (n == 0) {
        Serial.println("no networks found");
      } else {
        hasNetworks  = true;
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
  if(hasNetworks == false){
      ESP.restart();
  }
  else{
    if (WiFi.status() != WL_CONNECTED && isCorrectPassword == false) {
      startLocalWeb();
    }
  }


}

void turnOffWiFi() {
  WiFi.disconnect();
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
  String strTriggerParameter = "";
  //process trigger
  if (configScheduler.length() > 1 /*&& hasSensor == true*/) {
    StaticJsonDocument < 2048 > docTrigger;
    // parse a JSON array
    DeserializationError errorTrigger = deserializeJson(docTrigger, configScheduler);

    if (errorTrigger) {
      Serial.println("deserializeJson() failed xxx");
      strTriggerParameter = "ConfigError";
    } else {
      // extract the values
      JsonArray triggerList = docTrigger.as < JsonArray > ();
      gHasSpeed = false;
      for (JsonObject v: triggerList) {
          int valueStart = v["startTimer"];
          Serial.print("valueStart=");
          Serial.println(valueStart);
  
          int valueStop = v["stopTimer"];
          Serial.print("valueStop=");
          Serial.println(valueStop);
  
          int currentSeconds= getSeconds();
          Serial.print("currentSeconds=");
          Serial.println(currentSeconds);
          
          String action = v["action"];
          Serial.print("action=");
          Serial.println(action);
   
          if( valueStart <= currentSeconds && currentSeconds < valueStop){
            setSpeed(action, true);
            gHasSpeed = true;
            break;
          }
      }

      if(gHasSpeed == false){
        setSpeed("0", true);
      }
    }
  }
  
   if(hasReport == false){
      return ret;
   }
   
  if (WiFi.status() != WL_CONNECTED) {
    time_to_sleep_mode = 60;
    Serial.println("sendReport WiFi.status() != WL_CONNECTED");
    delay(5000); 
    ESP.restart();
    return false;
  }

  String localIP = WiFi.localIP().toString();
  if (localIP == "0.0.0.0") {
    time_to_sleep_mode = 60;
    Serial.println("sendReport  localIP= 0.0.0.0");
    delay(5000); 
    ESP.restart();
    return false;
  }

  WiFiClientSecure * client = new WiFiClientSecure;
  if (!client) {
    return false;
  }

  
  client -> setInsecure();
  HTTPClient http;
  String serverPath = serverConfig+ "?sensorName=Pwm&deviceID=" + deviceID + "&serialNumber=" + serialNumber +  "&release=" + releaseDate + "&uptime=" + String(gUptime)  + "&pwm=" + currentPwm;
 
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
              deviceID = id;
            }
          }
  
          bool hasSerialNumber = doc.containsKey("serialNumber");
          if (hasSerialNumber == true) {
            String serial_number = doc["serialNumber"];
            if (serial_number.length() > 1 && serial_number.length() < 64) {
              EEPROM.writeString(EEPROM_ADDRESS_SERIAL_NUMBER, serial_number);
              EEPROM.commit();
              serialNumber = serial_number;
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

        bool hasScheduler = doc.containsKey("schedulers");
        if (hasScheduler == true) {
            JsonArray schedulerList = doc["schedulers"];
            String strSchedulers = "";
            serializeJson(schedulerList, strSchedulers);
            Serial.print("strSchedulers=");
            Serial.println(strSchedulers);
            Serial.println("strSchedulers.length()=" + String(strSchedulers.length()));
            configScheduler = strSchedulers;
        }
    }
    retryTimeout = 0;
  } else {
     Serial.print("sendReport Error code: ");
     Serial.println(httpResponseCode);
      time_to_sleep_mode = TIME_TO_SLEEP;
      retryTimeout = retryTimeout + 1;
      //Timeout - https://github.com/esp8266/Arduino/issues/5137
      if(httpResponseCode == -11  ){
        http.end();
        delete client;
        delay(3000);
        Serial.println("sendReport retryTimeout=" + String(retryTimeout));
        if(retryTimeout > 3){
              ESP.restart();
        }
      }  else if(retryTimeout > 4){
          ESP.restart();
      }
  }
  // Free resources
  http.end();
  delete client;

  return ret;
}

bool getTimeZone( ) {
  bool ret = false;
  if (WiFi.status() != WL_CONNECTED) {
    time_to_sleep_mode = 60;
    Serial.println("getTimeZone WiFi.status() != WL_CONNECTED");
    delay(1000); 
    ESP.restart();
    return ret;
  }

  String localIP = WiFi.localIP().toString();
  if (localIP == "0.0.0.0") {
    time_to_sleep_mode = 60;
    Serial.println("getTimeZone  localIP= 0.0.0.0");
    delay(1000); 
    ESP.restart();
    return false;
  }

  WiFiClientSecure * client = new WiFiClientSecure;
  if (!client) {
    return ret;
  }

  
  client -> setInsecure();
  HTTPClient http;
  String serverPath = serverOffset + "?deviceID=" + deviceID;

  
  Serial.println(serverPath);

  http.setTimeout(60000);
  http.begin( * client, serverPath.c_str());

  // Send HTTP GET request
  int httpResponseCode = http.GET();

  if (httpResponseCode == 200) {
    String payload = http.getString();
   
    DynamicJsonDocument doc(256);

    DeserializationError error = deserializeJson(doc, payload);
    if (error) {
      Serial.println("deserializeJson() failed");
    } else {
        bool hasKey = doc.containsKey("gmtOffset");
        if (hasKey == true) {
          int gmtOffset = doc["gmtOffset"];
          gmtOffset_sec = gmtOffset;
          Serial.print("getTimeZone gmtOffset_sec=");
          Serial.println(gmtOffset_sec);
        }
    }   
  } else {
     Serial.print("getTimeZone Error code: ");
     Serial.println(httpResponseCode);
     ESP.restart();
  }
  // Free resources
  http.end();
  delete client;

  return ret;
}


bool sendError( bool hasError) {
   Serial.println("sendError  ");
  bool ret = false;
  if (WiFi.status() != WL_CONNECTED) {
    time_to_sleep_mode = 60;
    Serial.println("getTimeZone WiFi.status() != WL_CONNECTED");
    delay(1000); 
    ESP.restart();
    return ret;
  }

  String localIP = WiFi.localIP().toString();
  if (localIP == "0.0.0.0") {
    time_to_sleep_mode = 60;
   
    delay(1000); 
    ESP.restart();
    return false;
  }

  WiFiClientSecure * client = new WiFiClientSecure;
  if (!client) {
    return ret;
  }

  
  client -> setInsecure();
  HTTPClient http;
   String errorCode = "errorCode=0";
   if(hasError == true){
    errorCode = "errorCode=1";
  }
  String serverPath = serverError + "?deviceID=" + deviceID + "&" + errorCode;

  
  Serial.println(serverPath);

  http.setTimeout(60000);
  http.begin( * client, serverPath.c_str());

  // Send HTTP GET request
  int httpResponseCode = http.GET();
 
  if (httpResponseCode == 200) {
    String payload = http.getString();
  } else {
     Serial.print("sendError Error code: ");
     Serial.println(httpResponseCode);
     ESP.restart();
  }
  // Free resources
  http.end();
  delete client;

  return ret;
}


void printLocalTime(){
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
}

int getSeconds(){
  int seconds = 0;
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    delay(1000); 
    ESP.restart();
    return seconds;
  }
  seconds = timeinfo.tm_hour*(60*60);
  seconds = seconds + (timeinfo.tm_min*60);
  seconds = seconds + (timeinfo.tm_sec);
  return   seconds ;
}


void setup() {
  Serial.begin(115200);
  delay(500); //Take some time to open up the Serial Monitor
  
  Serial.println("Ver:8/Aug/2023");
  
  initSensor();
  
  initEEPROM();
  initWiFi();

  if(deviceID.length() > 0){
    getTimeZone();
  }

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  if(startEpchoTime == 0){
     startEpchoTime = getSeconds();
  }

 
}

void loop() {
   // printLocalTime();
   int currntEpchoTime =  getSeconds();
    gUptime=  currntEpchoTime - startEpchoTime;

   g_count = g_count +1;
   if((g_count > 60) || ( ( g_count> 15) && gHasSpeed == true )){
       sendReport(true); 
      g_count= 0;
   }else{
      sendReport(false); 
   }
   
   delay(1000);
}
