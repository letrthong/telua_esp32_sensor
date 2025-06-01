#include <WiFi.h>
#include <EEPROM.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
 
 
#define uS_TO_S_FACTOR 1000000ULL /* Conversion factor for micro seconds to seconds */
#define EEPROM_SIZE 512
#define TIME_TO_SLEEP 30

 
RTC_DATA_ATTR int countOn = 0;
 

// the LED is connected to GPIO 5
const int ledRelay01 = 17 ; 
const int ledRelay02 =  5; 
//const int ledAlarm =  19; 
const int ledFloatSwitch =  4; 

const int btnTop = 16;
const int btnBot = 18 ;

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

 

bool checkGPIO() { 
    bool has_data = false;
    delay(1000);
    int buttonState = digitalRead(btnTop);
    if (buttonState == HIGH) {
      //Serial.println("digitalRead fbtnTop HIGH");
      digitalWrite(ledRelay01, HIGH);
      has_data = true;
    } else {
      //Serial.println("digitalRead fbtnTop LOW");
      digitalWrite(ledRelay01, LOW);
    }
    

  buttonState = digitalRead(btnBot);
  if (buttonState == HIGH) {
    //Serial.println("digitalRead fbtnBot HIGH");
    digitalWrite(ledRelay02, HIGH);
    has_data = true;
  } else {
   // Serial.println("digitalRead fbtnBot LOW");
    digitalWrite(ledRelay02, LOW);
  }

  //  if (has_data == true) {
  //      digitalWrite(ledAlarm, HIGH);
  //  } else {
  //     digitalWrite(ledAlarm, LOW);
  //  }

   return has_data;

}

void sleepMinutes(int minutes) {
    int seconds = minutes*60;
    for( int second = 0; second < seconds; second ++) {
       delay(1000);
    }
}

void setup() {
  Serial.begin(115200);
  delay(1000); //Take some time to open up the Serial Monitor
  
  initGpio();
  

}

void loop() {
  //This is not going to be called
  if(checkGPIO() == true) {
    countOn  = countOn + 1;
  }

  // 60 minutes
  if (countOn > 60*1) {
    countOn = 0;
    Serial.println("sleepMinutes");
    sleepMinutes(15);
    ESP.restart();
  }
}
