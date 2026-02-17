#include <WiFi.h>
#include "time.h"
#include <EEPROM.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <esp_task_wdt.h>

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

 

String serverName = "https://telua.co/service/v1/esp32/scheduler";
String serverOffset = "https://telua.co/service/v1/esp32/gmtOffset"; 
// String btnStatus = "&b1=off&b2=off&al=off"; // Removed global String
String releaseDate = "16-Feb-2026";
String gWifiName = "";
String gVoltage = "220";
int gSignalStrength = 0; // Changed to int to avoid heap fragmentation
String gProtocol = "&protocol=RESTfulAPI";
int gPollingTime = 60; // Changed to int

bool gIsDefaultWifi = true;
String gDefaultWifname = "hcmus";
String gDefaultWifPass = "fetelxxx";

// String gDefaultWifname_telua = "telua";
// String gDefaultWifPass_telua = "13572468";
String gDefaultWifname_telua = "HO CHI MINH US";
String gDefaultWifPass_telua = "12345678";

// Timer2Channels or Timer3Channels
bool gHas2Channel = false;
bool gPWM = true;

// Timer3Channels
String gSensorName = "Pump";

int gUptime = 0;
unsigned long gUptimeCounter = 0;
int gPreUptime = 0;
int startEpchoTime = 0;

int EEPROM_ADDRESS_SSID = 0;
int EEPROM_ADDRESS_PASS = 32;
int EEPROM_ADDRESS_REMOTE_SSID = 48;
int EEPROM_ADDRESS_REMOTE_PASS = 64;
int EEPROM_ADDRESS_TIME_TO_SLEEP = 96; 
int EEPROM_ADDRESS_DEVICE_ID = 128;
int EEPROM_ADDRESS_SERIAL_NUMBER = 192;
int EEPROM_ADDRESS_TRIGGER = 256;

bool hasRouter = false;
RTC_DATA_ATTR int g_encryption_Type = WIFI_AUTH_OPEN;

bool hasRemoteRouter = false;
RTC_DATA_ATTR int g_remote_encryption_Type = WIFI_AUTH_OPEN;

bool hasSensor = false;
bool hasError = true;
RTC_DATA_ATTR int retryTimeout = 0;
bool g_isRestarting = false; // Flag to prevent race conditions during restart
int g_count = 60;
int time_to_sleep_mode = TIME_TO_SLEEP;
 
const char * ssid = "Telua_Timer_";
 
const char * password = "12345678";
String g_ssid = "";
unsigned long previousMillis = 0;
unsigned long interval = 60000;

unsigned long previousMillisLocalWeb = 0;
unsigned long intervalLocalWeb = 60000;

String g_ntpServer = "pool.ntp.org";
// 25200 = 7*60*60  +7
long gmtOffset_sec = 25200;
const int daylightOffset_sec = 0;
 
// the LED is connected to GPIO 5
bool hasGPIo = true;
int ledRelay01 = 17; 
int ledRelay02 = 5; 
int ledAlarm =  18; 
const int ledFloatSwitch =  4; 

const int ledWifiStatus = 2;  
// const int btnTop = 19 ( 18-> 19);
// const int btnBot = 16;

TaskHandle_t taskHandle;
void task1(void *parameter);

void initGpio()
{

    if(gHas2Channel == true)
    {
      ledRelay01 = 5 ;
      ledRelay02 = 18  ;
      ledAlarm = 17;
      gSensorName = "Timer2Channels";
      if(gPWM == true)
      {
        gSensorName = "PWM2Channels";
      }

      pinMode(ledRelay01, OUTPUT);
      pinMode(ledRelay02, OUTPUT);
      pinMode(ledAlarm, OUTPUT);
    }
    else
    {

     
      pinMode(ledRelay01, OUTPUT);
      pinMode(ledRelay02, OUTPUT);
      pinMode(ledAlarm, OUTPUT);
    }
   
   pinMode(ledWifiStatus, OUTPUT);
   //turnOnAll();

   turnOffAll();
}

void turnOffAll(){
  if(gHas2Channel == true)
  {
      digitalWrite(ledRelay01, LOW);
      digitalWrite(ledRelay02, LOW);
  }
  else
  {
    digitalWrite(ledRelay01, LOW);
    digitalWrite(ledRelay02, LOW);
    digitalWrite(ledAlarm, LOW);
  }
  
}


void turnOnAll(){
  if(gHas2Channel == true)
  {
      digitalWrite(ledRelay01, HIGH);
      digitalWrite(ledRelay02, HIGH);
  }
  else
  {
    digitalWrite(ledRelay01, HIGH);
    digitalWrite(ledRelay02, HIGH);
    digitalWrite(ledAlarm, HIGH);
  }

  delay(1000);
}

bool turnOnRelay(String action){
   if (g_isRestarting) return false; // Block action if restarting
   bool retCode  = false;
   
   if( action =="b1On"){
       Serial.println("turnOnRelay b1On");
       //delay(3000); 
       digitalWrite(ledRelay01, HIGH);
        retCode = true;
   }else  if( action =="b2On"){
      Serial.println("turnOnRelay b2On");
      //delay(3000); 
      digitalWrite(ledRelay02, HIGH);
       retCode = true;
   } else  if( action == "alOn"){
       Serial.println("turnOnRelay alOn");
      // delay(1000); 
       digitalWrite(ledAlarm, HIGH);
       retCode = true;
   } 

   return retCode;
}

bool turnOffRelay(String action){
   if (g_isRestarting) return false; // Block action if restarting
   bool retCode  = false;
   if( action =="b1Off"){
       digitalWrite(ledRelay01, LOW);
   }else  if( action =="b2Off"){
      digitalWrite(ledRelay02, LOW);
   } else  if( action == "alOff"){
       digitalWrite(ledAlarm, LOW);
   } 
   return retCode;
}

void restartDevice() {
  g_isRestarting = true; // Signal all tasks to stop touching hardware
  Serial.println("Restarting device...");
  Serial.flush(); // Flush before suspending to avoid deadlock if task1 holds Serial mutex

  // REMOVED vTaskSuspend: Suspending a task holding a mutex (e.g. inside malloc or Serial) 
  // causes Deadlock. It is safer to just let it run and block GPIO via g_isRestarting flag.
  
  turnOffAll(); // Turn off all relays to prevent power spikes
  delay(1000);  // Wait for power to stabilize
  
  // Unsubscribe from WDT to avoid panic during the final delay/restart phase
  esp_task_wdt_delete(NULL);
  ESP.restart();
}

void startSleepMode() {
  /*
   First we configure the wake up source
   We set our ESP32 to wake up every 5 seconds
   */
  esp_sleep_enable_timer_wakeup(time_to_sleep_mode * uS_TO_S_FACTOR);
  Serial.print("Setup ESP32 to sleep for every ");
  Serial.print(time_to_sleep_mode);
  Serial.println(" Seconds");

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
    esp_task_wdt_reset(); // Reset WDT to prevent crash during long configuration

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

    if (count >= 3) { // Giam thoi gian cho xuong 3 phut (thay vi 10 phut) de tranh bi "treo" qua lau
      server.close();
      WiFi.disconnect();
      delay(100);
      time_to_sleep_mode = 30;
      //startLocalWeb();
      restartDevice(); // Restart instead of Deep Sleep to keep the device active
      return;
    }

    if (client) { // If a new client connects,
      Serial.println("New Client."); // print a message out in the serial port
      String currentLine = ""; // make a String to hold incoming data from the client
      bool hasWrongFormat = false;
      String privateIpv4 = "";
      
      // Optimize: Reserve memory to prevent fragmentation during char-by-char addition
      header.reserve(2048);

      while (client.connected()) { // loop while the client's connected
        if (client.available()) { // if there's bytes to read from the client,
           esp_task_wdt_reset(); // Keep WDT happy during data transfer
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

                  String wifiPassword = info.substring(index + 10);
                  wifiPassword.replace(" ", "");

                  Serial.print("ssid=[");
                  Serial.print(ssid);
                  Serial.print("]");

                  Serial.print("password=[");
                  Serial.print(wifiPassword);
                  Serial.print("]");

                  if (ssid.length() > 0) {
                    if (wifiPassword.length() == 0) {
                      Serial.println("WIFI_AUTH_OPEN");
                      WiFi.begin(ssid);
                      wifiPassword = "12345678";
                    } else {
                      WiFi.begin(ssid, wifiPassword);
                    }

                    Serial.print("Connecting to WiFi ..");
                    int count = 0;
                    while (WiFi.status() != WL_CONNECTED) {
                      Serial.print('.');
                      esp_task_wdt_reset(); // Keep WDT happy while connecting
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

                      EEPROM.writeString(EEPROM_ADDRESS_PASS, wifiPassword);
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
              client.println("<body><h4>Telua Nen Tang Cho IoT- Telua IoT platform - 20-08-2025 </h4>");
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
        restartDevice();
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
        restartDevice();
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
  
  digitalWrite(ledWifiStatus, HIGH);

  WiFi.mode(WIFI_STA);

  String current_ssid = EEPROM.readString(EEPROM_ADDRESS_SSID);
  String current_pass = EEPROM.readString(EEPROM_ADDRESS_PASS);
  
  gWifiName = current_ssid;	
  if(gIsDefaultWifi == true)
  {
    current_ssid = gDefaultWifname;
    current_pass = gDefaultWifPass;
    gWifiName = "const " + current_ssid;
  }
  
  unsigned int length_of_ssid = current_ssid.length();
  g_ssid = current_ssid;
  
  
  gWifiName.replace(" ", "+");

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
        
        // Optimize String to avoid Heap fragmentation right from startup
        select_html = "";
        select_html.reserve(2048); // Reserve memory for Wifi list
        select_html += " <select  id=\"ssid\"  style=\"height:30px; width:120px;\"   name=\"ssid\">";
        
        for (int i = 0; i < n; ++i) {
          String SSID = WiFi.SSID(i);
          Serial.print("scanNetworks SSID=");
          Serial.println(SSID);
          if (i < 10) {
            select_html += "<option value=\""; select_html += SSID; select_html += "\">"; select_html += SSID; select_html += "</option>";
          }

            if (length_of_ssid > 0) {
              if (current_ssid.equals(SSID)) {
                hasRouter = true;
                g_encryption_Type = WiFi.encryptionType(i);
                gSignalStrength = WiFi.RSSI(i);
              } else if(gDefaultWifname_telua.equals(SSID)){
                hasRouter = true;
                g_encryption_Type = WiFi.encryptionType(i);
                gSignalStrength = WiFi.RSSI(i);

                current_ssid = gDefaultWifname_telua;
                current_pass  = gDefaultWifPass_telua;
                g_ssid = current_ssid;
                gWifiName = "const " + current_ssid;
                gWifiName.replace(" ", "+");
              }
              else if (remote_ssid.equals(SSID) && (hasRouter == false)  ) {
              
                g_remote_encryption_Type = WiFi.encryptionType(i);
                if(g_remote_encryption_Type != WIFI_AUTH_OPEN)
                {
                    if(remote_pass.length() > 1){
                        gSignalStrength = WiFi.RSSI(i);  
                        hasRemoteRouter = true;
                    }
                }
              }
            }
        }

        if (n < 1) {
          select_html += "<option value=\" \"> </option>";
        }
        select_html += " </select> <br>";
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
      esp_task_wdt_reset(); // Keep WDT happy
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
      if (g_remote_encryption_Type == WIFI_AUTH_OPEN) {
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
          restartDevice();
        }
      }
      Serial.print("Connecting to Remote WiFi ..");
      int count = 0;
      int retryTime = 30;
      while (WiFi.status() != WL_CONNECTED &&  (isConnecting == true)) {
        Serial.print('.');
        esp_task_wdt_reset(); // Keep WDT happy
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
          restartDevice();
        }
      } else {
        if(isConnecting == true){
           EEPROM.writeString(EEPROM_ADDRESS_REMOTE_SSID, "");
           EEPROM.commit();
           restartDevice();
        }
      }
    }
  }

  Serial.println(WiFi.localIP());
  // Neu khong co mang hoac ket noi that bai, vao che do AP (startLocalWeb) thay vi restart lien tuc
  if (WiFi.status() != WL_CONNECTED && isCorrectPassword == false) {
        if (gIsDefaultWifi == true) {
             Serial.println("Default WiFi not found. Restarting...");
             restartDevice();
        }
        startLocalWeb();
    }

  digitalWrite(ledWifiStatus, LOW);
  
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

  remote_pass = EEPROM.readString(EEPROM_ADDRESS_REMOTE_PASS);
  Serial.print("initEEPROM remote_pass=");
  Serial.println(remote_pass);
}

bool sendReport(bool hasReport) {
  bool ret = false;
  String strTriggerParameter = "";
  //process trigger
    bool hasBtn0 = false;
    bool hasBtn1 = false; 
    bool hasAl = false; 

  if (configScheduler.length() > 1 /*&& hasSensor == true*/) {
    // Use StaticJsonDocument because Task1 has large stack (40KB). Avoids Heap fragmentation.
    StaticJsonDocument<2048> docTrigger;
    // parse a JSON array
    DeserializationError errorTrigger = deserializeJson(docTrigger, configScheduler);

    if (errorTrigger) {
      Serial.println("deserializeJson() failed xxx");
      strTriggerParameter = "ConfigError";
    } else {
      // extract the values
      JsonArray triggerList = docTrigger.as < JsonArray > ();
    
      for (JsonObject v: triggerList) {
          int valueStart = v["startTimer"];
          //Serial.print("valueStart=");
          //Serial.println(valueStart);
  
          int valueStop = v["stopTimer"];
          //Serial.print("valueStop=");
          //Serial.println(valueStop);
  
          int currentSeconds= getSeconds();
          //Serial.print("currentSeconds=");
          //Serial.println(currentSeconds);
          
          String action = v["action"];
          Serial.print("action=");
          Serial.println(action);
   
          if( valueStart <= currentSeconds && currentSeconds < valueStop){
               Serial.println("turn on");
               if( action.indexOf("b1") > -1){
                 hasBtn0 =  true;
               } else if( action.indexOf("b2") > -1){
                 hasBtn1 =  true;
               } else if( action.indexOf("al") > -1){
                 hasAl =  true;
               }
          }
      }

    
    }
  }
  
  // Use local String instead of global to free memory after use
  String localBtnStatus = "";
  localBtnStatus.reserve(64);
  
  if(hasBtn0 == true){
      turnOnRelay("b1On");
      localBtnStatus += "&b1=on";
  } else {
    turnOffRelay("b1Off");
      localBtnStatus += "&b1=off";
  }

  if(hasBtn1 == true){
      turnOnRelay("b2On");
      localBtnStatus += "&b2=on";
  } else {
    turnOffRelay("b2Off");
      localBtnStatus += "&b2=off";
  }

  if(gHas2Channel == false){
      if(hasAl == true){
        turnOnRelay("alOn");
        localBtnStatus += "&al=on";
      } 
      else
       {
        turnOffRelay("alOff");
        localBtnStatus += "&al=off";
      }


  }
  

   if(hasReport == false){
      return ret;
   }
   
  if (WiFi.status() != WL_CONNECTED) {
    // Do not restart immediately, let loop() handle 10-minute timeout
    Serial.println("sendReport: WiFi not connected, skipping...");
    return false;
  }

  digitalWrite(ledWifiStatus, HIGH);

  String localIP = WiFi.localIP().toString();
  if (localIP == "0.0.0.0") {
    Serial.println("sendReport: Invalid IP 0.0.0.0");
    return false;
  }

  // Use Stack allocation instead of Heap (new/delete) to avoid memory fragmentation
  WiFiClientSecure client;

   gSignalStrength = WiFi.RSSI(); 
  
  client.setInsecure();
  HTTPClient http;
  
  // Optimize String concatenation to reduce heap fragmentation
  String serverPath;
  serverPath.reserve(512);
  char buf[32]; // Buffer for number conversion

  serverPath += serverName;
  serverPath += "?sensorName="; serverPath += gSensorName;
  serverPath += "&deviceID="; serverPath += deviceID;
  serverPath += "&serialNumber="; serverPath += serialNumber;
  serverPath += "&release="; serverPath += releaseDate;
  
  serverPath += "&uptime="; itoa(gUptime, buf, 10); serverPath += buf;
  
  serverPath += localBtnStatus;
  serverPath += "&wiFiName="; serverPath += gWifiName;
  serverPath += "&volt="; serverPath += gVoltage;
  serverPath += "&signalStrength="; itoa(gSignalStrength, buf, 10); serverPath += buf;
  serverPath += gProtocol;
  serverPath += "&pollingTime="; itoa(gPollingTime, buf, 10); serverPath += buf;
  serverPath += "&ntpServer="; serverPath += g_ntpServer;

  uint32_t freeHeap = ESP.getFreeHeap();
  uint32_t totalHeap = ESP.getHeapSize();
  float usedRam = (float)(totalHeap - freeHeap) / totalHeap * 100.0;
  Serial.printf("Heap: Free %u / %u (%.1f%% Used) | Min Free: %u\n", freeHeap, totalHeap, usedRam, ESP.getMinFreeHeap());
  
  Serial.println(serverPath);

  http.setTimeout(55000); // Reduce to 55s to leave enough time for Watchdog (75s)
  http.begin(client, serverPath.c_str());

  // Send HTTP GET request
  int httpResponseCode = http.GET();

  if (httpResponseCode == 200) {
     digitalWrite(ledWifiStatus, LOW);
    //        Serial.print("HTTP Response code: ");
    //        Serial.println(httpResponseCode);
    //https://arduinojson.org/v6/doc/upgrade/
    // Use StaticJsonDocument because Task1 has large stack (40KB). Avoids Heap fragmentation.
    StaticJsonDocument<2048> doc;

    // FIX: Use getStream() instead of getString() to avoid Heap fragmentation after long uptime
    DeserializationError error = deserializeJson(doc, http.getStream());
    if (error) {
      Serial.print("deserializeJson() failed: ");
      Serial.println(error.c_str()); // Print error reason (e.g. NoMemory, InvalidInput)
      restartDevice();
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
            EEPROM.writeString(EEPROM_ADDRESS_REMOTE_PASS, remote_pass);
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
     Serial.print("Error code: ");
     Serial.println(httpResponseCode);
      time_to_sleep_mode = TIME_TO_SLEEP;
      retryTimeout = retryTimeout + 1;
      configScheduler  = "";
      //Timeout - https://github.com/esp8266/Arduino/issues/5137
      if(httpResponseCode == -11){
        http.end();
        // delete client; // No need to delete because stack is used
        delay(3000);
        Serial.println("sendReport retryTimeout=" + String(retryTimeout));
         if(hasGPIo == true){
              if(retryTimeout > 3){
                 restartDevice();
              }else{
                return ret;
            } 
         }else{
              restartDevice();
         }
       
      } else {
         if(retryTimeout > 4){
            restartDevice();
          }
      }
  }
  // Free resources
  http.end();
  client.stop(); // Explicitly close the SSL connection
  // delete client; // Automatically destroyed when out of scope

  return ret;
}

bool getTimeZone( ) {
  bool ret = false;
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("getTimeZone: WiFi not connected, skipping...");
    return ret;
  }

  String localIP = WiFi.localIP().toString();
  if (localIP == "0.0.0.0") {
    Serial.println("getTimeZone: Invalid IP 0.0.0.0");
    return false;
  }

  WiFiClientSecure client;

  
  client.setInsecure();
  HTTPClient http;
  String serverPath = serverOffset + "?deviceID=" + deviceID;

  
  Serial.println(serverPath);

  http.setTimeout(60000);
  http.begin(client, serverPath.c_str());

  // Send HTTP GET request
  int httpResponseCode = http.GET();

  if (httpResponseCode == 200) {
    String payload = http.getString();
   
    // Use StaticJsonDocument to avoid Heap fragmentation
    StaticJsonDocument<256> doc;

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

        hasKey = doc.containsKey("ntpServer");
        if (hasKey == true) {
          String ntpServer = doc["ntpServer"];
          g_ntpServer= ntpServer;
          Serial.print("getTimeZone g_ntpServer=");
          Serial.println(g_ntpServer);
        }
        else{
          Serial.println("getTimeZone No ntpServer ");
        }
    }   
  } else {
     Serial.print("getTimeZone Error code: ");
     Serial.println(httpResponseCode);
  }
  // Free resources
  http.end();

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
    restartDevice();
    return seconds;
  }
  seconds = timeinfo.tm_hour*(60*60);
  seconds = seconds + (timeinfo.tm_min*60);
  seconds = seconds + (timeinfo.tm_sec );
  return  seconds ;
}

void init_ntp() {
  if (deviceID.length() > 0) {
    Serial.println("Fetching timezone from server...");
    esp_task_wdt_reset(); // Reset WDT before long HTTP call
    getTimeZone();  // Updates gmtOffset_sec and daylightOffset_sec if needed
  }

  const char* ntpServers[] = {
    g_ntpServer.c_str(),       // Primary NTP server (e.g., "pool.ntp.org")
    "time.google.com",         // Backup server 1
    "vn.pool.ntp.org",         // Backup server 2
    "asia.pool.ntp.org",        // Backup server 3
    "time.cloudflare.com"       // Backup server 4
  };

  struct tm timeinfo;
  const int maxRetries = 10;
  bool timeSynced = false;

  for (int i = 0; i < sizeof(ntpServers) / sizeof(ntpServers[0]); i++) {
    Serial.printf("Trying to sync time with NTP server: %s\n", ntpServers[i]);
    esp_task_wdt_reset(); // Reset WDT before trying new server
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServers[i]);

    for (int retry = 0; retry < maxRetries; retry++) {
      if (getLocalTime(&timeinfo)) {
        Serial.println("Time synchronization successful!");
        Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
        timeSynced = true;
        g_ntpServer = ntpServers[i];
        break;
      }
      Serial.print(".");
      esp_task_wdt_reset(); // Reset WDT while waiting for NTP
      delay(1000);
    }

    if (timeSynced) {
      break;
    }
    Serial.println("\n  Failed to sync with this server. Trying next...");
  }

  if (!timeSynced) {
    Serial.println("All NTP sync attempts failed. Please check Wi-Fi or UDP port 123.");
    restartDevice();
  }
}

void setup() {
  
  Serial.begin(115200);
  Serial.print("Setup/Loop running on Core: ");
  Serial.println(xPortGetCoreID());
  
  initGpio();
  
  for(int i = 0; i<4 ;i++){
      digitalWrite(ledWifiStatus, HIGH);
      delay(500); 
      digitalWrite(ledWifiStatus, LOW);
      delay(500); 
  }
  
  Serial.println("Ver:16/Feb/2026");
  
  initEEPROM();

  esp_task_wdt_deinit();

  esp_task_wdt_config_t config = {0};
  config.timeout_ms = 75000; // Increase WDT to 75s (> 55s HTTP timeout + overhead)
  config.trigger_panic = true;
  // config.idle_core_mask = (1 << 0) | (1 << 1); // Comment out to avoid errors on single-core chips
 
  esp_err_t err = esp_task_wdt_init(&config); // Initialize ESP32 Task WDT
  if (err != ESP_OK) {
    Serial.printf("Task WDT Init Failed: %s\n", esp_err_to_name(err));
  } else {
    Serial.println("Task WDT Init Success");
  }
  
  esp_task_wdt_add(NULL);   // Subscribe to the Task WDT

  initWiFi();

  if (WiFi.status() == WL_CONNECTED) {
    for(int i = 0; i<3 ;i++){
      digitalWrite(ledWifiStatus, HIGH);
      delay(500); 
      digitalWrite(ledWifiStatus, LOW);
      delay(500); 
    }
  }

  // Create task1 pinned to Core 1 (Application Core) to avoid interfering with WiFi on Core 0
  xTaskCreatePinnedToCore(
    task1,       // Task function pointer
    "Task1",     // Task name
    10000,        // Stack depth in words
    NULL,        // Task parameter
    2,           // Task priority
    &taskHandle, // Task handle
    1            // Core ID (0 or 1)
  );
}

void checkMemory() {
  // Monitor Memory: Check every 5 minutes (300s)
  if (gUptimeCounter % 300 == 0) {
      uint32_t freeHeapLoop = ESP.getFreeHeap();
      uint32_t totalHeapLoop = ESP.getHeapSize();
      if (freeHeapLoop < (totalHeapLoop * 0.1)) {
          Serial.printf("Memory Critical: Used > 90%% (Free: %u / %u). Restarting...\n", freeHeapLoop, totalHeapLoop);
          restartDevice();
      }
  }
}

void checkTaskStuck() {
  // Check for "Dead Task" every 10 minutes (600s)
  if (gUptimeCounter % 600 == 0) {
      if (gPreUptime != gUptime ) {
        gPreUptime = gUptime; 
      } else {
          Serial.println("Loop: Task1 appears stuck (Uptime not changing). Restarting...");
          restartDevice();  
      }
  }
}

void checkWiFiConnection() {
  // Restart if the device can not access the internet after 5 minutes (warm-up time)
  if (gUptimeCounter >= 300){
    if (WiFi.status() != WL_CONNECTED) {
          Serial.println("Loop: WiFi not connected, restarting...");
          restartDevice();
      }
   }
}

void loop() 
{
  static unsigned long previousLoopMillis = 0;
  unsigned long currentMillis = millis();

  // TEST: Restart Trigger for debugging
  // Uncomment to force restart after ~10 minutes to test stability
  // static long randomLimit = random(60, 1000);
  // static bool isPrinted = false;
  // if (!isPrinted) {
  //     Serial.printf("DEBUG: Random Restart Limit set to: %ld seconds\n", randomLimit);
  //     isPrinted = true;
  // }

  // if ( gUptimeCounter > randomLimit) {
  //    restartDevice();
  // }
  

  // Run logic every 1000ms (1 second)
  if (currentMillis - previousLoopMillis >= 1000) {
      previousLoopMillis = currentMillis;

      gUptimeCounter = gUptimeCounter + 1;
  
      // Prevent overflow: Reset to 300 (not 0) to keep ">= 300" logic active
      // Although unsigned long takes ~136 years to overflow, this is a safe guard.
      if (gUptimeCounter > 2000000000) {
          gUptimeCounter = 300;
      }

      // Modularized checks for better readability
      checkMemory();
      checkTaskStuck();
      checkWiFiConnection();
       
      // Log WDT reset (keep inside 1s interval to avoid spamming serial)
      struct tm timeinfo;
      if(getLocalTime(&timeinfo)){
          Serial.printf("esp_task_wdt_reset %02d:%02d:%02d gUptimeCounter=%d\n", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec ,gUptimeCounter);
      } else {
          Serial.println("esp_task_wdt_reset");
      }
  }

  // Kick the dog frequently
  esp_task_wdt_reset();
  
  // Yield to other tasks
  delay(10);
}


void task1(void *parameter) {  
  // 1. Register task1 with Watchdog
  esp_task_wdt_add(NULL);

  Serial.print("Task1 running on Core: ");
  Serial.println(xPortGetCoreID());
  
  init_ntp();

  if(startEpchoTime == 0){
    startEpchoTime = getSeconds();
  }

  // Force immediate report on startup
  unsigned long lastReportTime = millis() - (gPollingTime * 1000);
  unsigned long lastTriggerTime = 0;

  while (1) {
    unsigned long currentMillis = millis();
    
    // Update Uptime (Gom logic nay ra ngoai de tranh lap lai code)
    int currntEpchoTime = getSeconds();
    gUptime = currntEpchoTime - startEpchoTime;
    if(gUptime < 0) { 
       startEpchoTime = currntEpchoTime; 
       gUptime = 0; 
    }

    // 1. Check Report (Priority - every gPollingTime seconds)
    if (currentMillis - lastReportTime >= (gPollingTime * 1000UL))
    {
        lastReportTime = currentMillis;
        
        esp_task_wdt_reset();
        unsigned long start = millis();
        sendReport(true); 
        Serial.printf("sendReport took: %lu ms\n", millis() - start);
        
        // FIX: Update currentMillis immediately to prevent logic drift after long HTTP call
        currentMillis = millis();
    }
    
    // 2. Check Triggers (every 1 second, non-blocking)
    if (currentMillis - lastTriggerTime >= 1000)
    {
        lastTriggerTime = currentMillis;
        sendReport(false); 
    }
    
    // 3. Yield to OS and Watchdog (Non-blocking)
    esp_task_wdt_reset();
    vTaskDelay(pdMS_TO_TICKS(10)); // Delay 10ms (chuan FreeRTOS)
  }
}
