#include <esp_task_wdt.h>

                 
// WDT Timeout in seconds
TaskHandle_t taskHandle;

void setup() {
  Serial.begin(115200);
  Serial.println("Setup started.");
  delay(2000);
  esp_task_wdt_config_t  config;
  config.timeout_ms = (5 * 1000);
  config.trigger_panic = true;

    
  
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
    Serial.println("LOOP started ! ");
    delay(1000);
    // Kick the dog
    esp_task_wdt_reset();
}


void task1(void *parameter) {
   int seconds = 0;
  int hours = 0;
  int minutes = 0;
  while (1) {
    Serial.print("MCU hang event!!! seconds: ");
    Serial.println(seconds);
    seconds = seconds+ 1;
    if(seconds > 60){
      seconds = 0;
      minutes = minutes + 1;
    }

     if(minutes > 60){
        minutes= 0;
        hours = hours +1;
     }

     if(hours > 24){
        hours = 0;
     }
    delay(1000);
  }
}