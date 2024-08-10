#include <esp_task_wdt.h>

#define WDT_TIMEOUT 600                   
// WDT Timeout in seconds

void setup() {
  Serial.begin(115200);
  Serial.println("Setup started.");
  delay(2000);
  esp_task_wdt_config_t  config;
  config.timeout_ms =WDT_TIMEOUT*1000;
  config.trigger_panic = true;
  
  esp_task_wdt_init(&config); // Initialize ESP32 Task WDT
  esp_task_wdt_add(NULL);               // Subscribe to the Task WDT
}

void loop() {
  Serial.println("LOOP started ! ");
  for (int i = 0; i <= 10; i++) {
    Serial.print("Task: ");
    Serial.println(i);
    delay(1000);
    // Kick the dog
    esp_task_wdt_reset();
  }

  int count = 0;
  while (1) {
     Serial.print("MCU hang event!!!: ");
    Serial.println(count);
    count = count+ 1
    delay(1000);
  }
}