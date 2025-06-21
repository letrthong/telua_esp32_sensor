#include <WiFi.h>
#include <EEPROM.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
 
 
#define uS_TO_S_FACTOR 1000000ULL /* Conversion factor for micro seconds to seconds */
 
 
// the LED is connected to GPIO 5
const int ledRelay01 = 17 ; 
const int ledRelay02 =  5; 
//const int ledAlarm =  19; 
const int ledFloatSwitch =  4; 

const int btnTop = 16;
const int btnBot = 18 ;

int g_count_is_running =0 ;
int g_count_is_stopping =0 ;

void initGpio(){
    pinMode(ledRelay01, OUTPUT);
    pinMode(ledRelay02, OUTPUT);
   // digitalWrite(ledAlarm, OUTPUT);  
    turnOffAll();

    pinMode(ledFloatSwitch, OUTPUT);  
    pinMode(btnTop, INPUT); 
    pinMode(btnBot, INPUT);  
    digitalWrite(ledFloatSwitch, HIGH);
    delay(1000);
}

void turnOffAll(){
   digitalWrite(ledRelay01, LOW);
   digitalWrite(ledRelay02, LOW);
  // digitalWrite(ledAlarm, LOW);
}

 

bool checkGPIO_BOT() { 
  bool has_data = false;
  int buttonState = digitalRead(btnBot);
  if (buttonState == HIGH) {
     Serial.println("checkGPIO_BOT fbtnBot HIGH");
    digitalWrite(ledRelay02, HIGH);
    has_data = true;
  } else {
   // Serial.println("digitalRead fbtnBot LOW");
    digitalWrite(ledRelay02, LOW);
  }
 
   return has_data;

}

bool checkGPIO_TOT() { 
    bool has_data = false;
    int buttonState = digitalRead(btnTop);
    if (buttonState == HIGH) {
      Serial.println("checkGPIO_TOT fbtnTop HIGH");
      digitalWrite(ledRelay01, HIGH);
      has_data = true;
    } else {
      //Serial.println("digitalRead fbtnTop LOW");
      digitalWrite(ledRelay01, LOW);
    }
 
   return has_data;

}

void sleepMinutes(int minutes) {
     Serial.println("sleepMinutes");
    int seconds = minutes*60;
    for( int second = 0; second < seconds; second ++) {
       delay(1000);
    }
}

void setup() {
  Serial.begin(115200);
  delay(1000); //Take some time to open up the Serial Monitor
  
  initGpio();
  g_count_is_running = 0;
  g_count_is_stopping = 0;
}

void loop() {
  delay(1000);
  //This is not going to be called
  bool has_data_bot = false;
  // 30 minutes -btn2
  has_data_bot = checkGPIO_BOT();
   
  bool has_data_tot = false;
  // 15 minutes -btn1
  has_data_tot = checkGPIO_TOT();


  if (has_data_bot== true || has_data_bot == true ) {
    g_count_is_running = g_count_is_running + 1; 
    g_count_is_stopping = -2;
  }  else {
    g_count_is_running = 0;
    g_count_is_stopping = g_count_is_stopping + 1;
  }

  if(g_count_is_stopping < 0){
    Serial.println("loop sleepMinutes  g_count_is_stopping < 0");
    turnOffAll();
    digitalWrite(ledFloatSwitch, LOW);
    sleepMinutes(120);
    ESP.restart();
  }
   
  // 15 minutes
  if (g_count_is_running> (15*60)) {
    Serial.println("loop sleepMinutes");
    turnOffAll();
    digitalWrite(ledFloatSwitch, LOW);
    sleepMinutes(60);
    ESP.restart();
  } else {
      if (g_count_is_stopping> (10*60) ) {
        Serial.println("loop sleepMinutes");
        turnOffAll();
        digitalWrite(ledFloatSwitch, LOW);
        sleepMinutes(60);
        ESP.restart();
      }
  }
}
