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

const char* ssid     = "Telua_SGP4x_";
const char* password = "12345678";
String g_ssid = "";
String ssid_list = "";

void intGpio(){
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);

  pinMode(buzzerPin, OUTPUT);
  digitalWrite(buzzerPin, LOW);
  
}

void turnOffLed(){
    //Serial.println("turnOffLed");
    digitalWrite(ledPin, LOW);
}


void turnOnBuzzer(){
  //Serial.println("turnOnBuzzer");
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

void startLocalWeb(){
    WiFi.mode(WIFI_AP_STA);
    WiFiServer server(80);
     Serial.print("Setting AP (Access Point)…");
  // Remove the password parameter, if you want the AP (Access Point) to be open
    IPAddress local_ip(192,168,0,1);
    IPAddress gateway(192,168,0,1);
    IPAddress subnet(255,255,255,0);
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
   bool hasRestart = false;
    while(1){
       WiFiClient client = server.available(); 
       unsigned long currentMillis = millis();
       if(currentMillis - previousMillis >=interval){
           previousMillis = currentMillis;
            Serial.println("waiting connection");
            count = count +1;
            if(hasConnection == true  ){
              hasConnection = false;
               count = 5;
            }
       }

       if(  count >= 6){
           server.close();
           WiFi.disconnect();
           delay(100);
           ESP.restart(); 
           return;
       }
       
       if (client) {                             // If a new client connects,
          Serial.println("New Client.");          // print a message out in the serial port
          String currentLine = "";                // make a String to hold incoming data from the client
          bool hasWrongFormat = false;
          String privateIpv4 = "";
          while (client.connected()) {            // loop while the client's connected
            if (client.available()) {             // if there's bytes to read from the client,
              char c = client.read();             // read a byte, then
  //            Serial.write(c);                    // print it out the serial monitor
              header += c;
              if (c == '\n') {                    // if the byte is a newline character
                // if the current line is blank, you got two newline characters in a row.
                // that's the end of the client HTTP request, so send a response:
                if (currentLine.length() == 0) {
                  // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
                  // and a content-type so the client knows what's coming, then a blank line:
                  client.println("HTTP/1.1 200 OK");
                  client.println("Content-type:text/html");
                  client.println("Connection: close");
                  client.println();
                  
                  int index  = header.indexOf("/router_info?ssid=");
                  if ( index>= 0) {
                      String parameters =  header.substring(index + 18);
                      index  = parameters.indexOf("HTTP");
                      
                      if(index > 0){
                         String  info =  parameters.substring(0,index);
                         Serial.println("/router_info");
                         Serial.println(info);
                         index = info.indexOf("&password="); 
                          
                         String ssid = info.substring(0,index);
                         String passowrd= info.substring(index+10);
                         passowrd.replace(" ",  "");
                         
                         Serial.print("ssid=[");
                         Serial.print(ssid);
                         Serial.print("]");
                         
                         Serial.print("passowrd=[");
                         Serial.print(passowrd);
                         Serial.print("]");
  
                        if(ssid.length()>0 && passowrd.length() >= 8){
                    
                             WiFi.begin(ssid, passowrd);
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

                             if(WiFi.status() == WL_CONNECTED){
                                Serial.println(WiFi.localIP());
                                privateIpv4  =  WiFi.localIP().toString().c_str();
                                hasConnection = true;
                                 EEPROM.writeString(EEPROM_ADDRESS_SSID, ssid);
                                 EEPROM.commit();
                                
                                 EEPROM.writeString(EEPROM_ADDRESS_PASS, passowrd);
                                 EEPROM.commit();
                             }else{
                               hasWrongFormat = true;
                             }
                        }else{
                          hasWrongFormat = true;
                        }
                         
                     }
                  }  
                  
                  // Display the HTML web page
                  client.println("<!DOCTYPE html><html>");
                  client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
                  client.println("<link rel=\"icon\" href=\"data:,\">");   
                  // Web Page Heading 
                  client.println("<body><h4>Telua Nen Tang Cho IoT- Telua IoT platform</h4>");
                  if(serialNumber.length() >0){
                       client.println("<p>Serial Number=" + serialNumber  + "</p>");
                  }

                  if(g_ssid.length() >0){
                       client.println("<p>SSID=" + g_ssid  + "</p>");
                  }
 
                  if(ssid_list.length() >0){
                       client.println("<p> Danh sach SSID - SSID list = [" + ssid_list  + "]</p>");
                  }

                  client.println("<form action=\"/router_info\"  method=\"get\">");
                  client.println("<label style=\"color:blue;\">SSID cua Wi-Fi - SSID of Wi-Fi</label><br>");
                  client.println("<input type=\"text\" style=\"height:25px;\"  id=\"ssid\" name=\"ssid\" value=\"\"><br>");
                  client.println("<label>Mat Khau cua Wi-Fi - Password of Wi-Fi</label><br>");
                  client.println("<input type=\"text\" style=\"height:25px;\" id=\"password\" name=\"password\" value=\"\"><br>");
                  client.println("<input type=\"submit\" style=\"margin-top:20px; height:40px;\"  value=\"Xac Nhan - Submit\">");
                  client.println("</form>");
                  if( hasWrongFormat == true){
                     client.println("<p>Xin kiem tra lai SSID va Mat Khau cua Wi-Fi</p>");
                     client.println("<p>Please recheck SSID and password of Wi-Fi</p>");
                  }else{
                      if(privateIpv4.length() >0){
                         client.println("<p  style=\"color:red;\"> Thiet bi co the ket noi Internet voi IPv4=" + privateIpv4  + "</p>");
                         client.println("<p  style=\"color:red;\"> The device can connect the inernet with IPv4=" + privateIpv4  + "</p>");

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
              } else if (c != '\r') {  // if you got anything else but a carriage return character,
                currentLine += c;      // add it to the end of the currentLine
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
    turnOffWiFi();
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
