#include <esp_task_wdt.h>

                 
// WDT Timeout in seconds
TaskHandle_t taskHandle;

const int ledRelay01 = 5; 
const int ledRelay02 = 18; 
const int ledAlarm =  19; 


void intGpio(){
    pinMode(ledRelay01, OUTPUT);
    pinMode(ledRelay02, OUTPUT);
    pinMode(ledAlarm, OUTPUT);
   turnOffAll();
}

void turnOffAll(){
   digitalWrite(ledRelay01, LOW);
   digitalWrite(ledRelay02, LOW);
   digitalWrite(ledAlarm, LOW);
}

void turnOnAll(){
     digitalWrite(ledRelay01, HIGH);
    digitalWrite(ledRelay02, HIGH);
    digitalWrite(ledAlarm, HIGH);
}



void resetGPIO(){
  Serial.println("rerestGPIO");
  digitalWrite(ledRelay01, HIGH);
  delay(10000);
  digitalWrite(ledRelay01, LOW);

  digitalWrite(ledRelay02, HIGH);
  delay(10000);
  turnOnAll();
 
  turnOffAll();
}



void setup() {
  Serial.begin(115200);
  Serial.println("Setup started.");
  delay(1000);
  esp_task_wdt_config_t  config;
  config.timeout_ms = (5 * 1000);
  config.trigger_panic = true;

  intGpio();
  resetGPIO();

  esp_task_wdt_init(&config); // Initialize ESP32 Task WDT
  esp_task_wdt_add(NULL);   // Subscribe to the Task WDT


  // Create task1
  xTaskCreate(
    task1,       // Task function pointer
    "Task1",     // Task name
    2000,        // Stack depth in words
    NULL,        // Task parameter
    2,           // Task priority
    &taskHandle  // Task handle
  );
}

void loop() {
    //Serial.println("LOOP started ! ");
    delay(1000);
    // Kick the dog
    esp_task_wdt_reset();
}


void task1(void *parameter) {
   int seconds = 0;
  int hours = 0;
  int minutes = 0;
  int count  = 0;
  while (1) {
    Serial.print("seconds=");
    Serial.print(seconds);
    Serial.print(" ,minutes=");
    Serial.print(minutes);
    Serial.print(" ,hours=");
    Serial.println(hours);
    seconds = seconds+ 1;
    if(seconds > 60){
      seconds = 0;
      minutes = minutes + 1;
      //resetGPIO();
    }

     if(minutes > 60){
        minutes= 0;
        hours = hours +1;
     }

     if(hours > 12){
        hours = 0;
        resetGPIO();
        count  = count +1;
     }


    delay(1000);

    //a week
    if(count> 14)
    {
      ESP.restart();
    }
    
  }
}