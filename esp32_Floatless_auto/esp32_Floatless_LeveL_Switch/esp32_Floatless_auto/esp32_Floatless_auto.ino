#include <WiFi.h>
#include <EEPROM.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
 
 
#define uS_TO_S_FACTOR 1000000ULL /* Conversion factor for micro seconds to seconds */
#define EEPROM_SIZE 512
#define TIME_TO_SLEEP 30

RTC_DATA_ATTR int bootCount = 0;

bool hasSensor = false;
bool hasError = true;
RTC_DATA_ATTR int retryTimeout = 0;

int time_to_sleep_mode = TIME_TO_SLEEP;

 
 
// the LED is connected to GPIO 5
bool hasGPIo = true;
const int ledRelay01 = 17 ; 
const int ledRelay02 =  5; 
const int ledAlarm =  19; 
const int ledFloatSwitch =  4; 

const int btnTop = 16;
const int btnBot = 18 ;

void intGpio(){
    pinMode(ledRelay01, OUTPUT);
    pinMode(ledRelay02, OUTPUT);
    digitalWrite(ledAlarm, OUTPUT);  
    turnOffAll();

    pinMode(ledFloatSwitch, OUTPUT);  
    pinMode(btnTop, INPUT); 
    pinMode(btnBot, INPUT);  
    digitalWrite(ledFloatSwitch, HIGH);
    delay(5000);
}

void turnOffAll(){
   digitalWrite(ledRelay01, LOW);
   digitalWrite(ledRelay02, LOW);
   digitalWrite(ledAlarm, LOW);
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

void checkGPIO(){
    delay(1000);
    int buttonState = digitalRead(btnTop);
    if (buttonState == HIGH) {
      Serial.println("digitalRead fbtnTop HIGH");
      digitalWrite(ledRelay01, HIGH);
    } else {
      Serial.println("digitalRead fbtnTop LOW");
      digitalWrite(ledRelay01, LOW);
    }
    

  buttonState = digitalRead(btnBot);
  if (buttonState == HIGH) {
    Serial.println("digitalRead fbtnBot HIGH");
    digitalWrite(ledRelay02, HIGH);
  } else {
    Serial.println("digitalRead fbtnBot LOW");
    digitalWrite(ledRelay02, HIGH);
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

  
  initGPIO();
 
  //Print the wakeup reason for ESP32
  print_wakeup_reason();
  //startSleepMode();

}

void loop() {
  //This is not going to be called
  checkGPIO();
}
