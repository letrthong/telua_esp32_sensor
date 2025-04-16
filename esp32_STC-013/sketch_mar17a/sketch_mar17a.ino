// EmonLibrary - OpenEnergyMonitor.org | GNU GPL V3 License
#include <Wire.h>
#include "EmonLib.h"                   // Include Emon Library
#include <driver/adc.h>
#include <Arduino.h>

EnergyMonitor emon1;                   // Create an instance
#define ADC_INPUT 34                   // ADC input pin
#define ADC_BITS 12                     // Set ADC resolution to 12-bit
#define ADC_COUNTS (1 << ADC_BITS)      // ADC step calculation (0-4095)
//#define BURDEN_RESISTOR 22.0            // Burden resistor (Ohms)
#define BURDEN_RESISTOR 42     

float voltage = 220.00;  // AC mains voltage (adjust based on your region)
float powerFactor = 0.95; // Estimated power factor

double amps;  
unsigned long lastMeasurement = 0;  
int counter = 0;

const int led01 = 0;  
const int led02 = 2;  

void setup() {
    Serial.begin(115200);
    pinMode(led01, OUTPUT);
    pinMode(led02, OUTPUT);

    digitalWrite(led01, LOW);
    digitalWrite(led02, LOW);

    analogReadResolution(ADC_BITS);    // Set ADC to 12-bit resolution
    emon1.current(ADC_INPUT, BURDEN_RESISTOR);  // Initialize current sensor with calibration
}

void loop() {
     amps = emon1.calcIrms(1480); 
            
      Serial.print("Measured Current: ");
      Serial.print(amps);
      Serial.println(" A");
      
      if (amps > 1.35) {
        amps = amps - 1.35;

      } else {
         amps = 0.0;
      }

      Serial.print("Measured Current: ");
      Serial.print(amps);
      Serial.println(" A");
      
      // Calculate Power (Watts)
      float power = amps * voltage * powerFactor;
      Serial.print("Estimated Power: ");
      Serial.print(power);
      Serial.println(" W");
        

    // LED status toggling
    delay(500);
    digitalWrite(led01, LOW);
    digitalWrite(led02, HIGH);
    delay(500);
    digitalWrite(led02, LOW);
    digitalWrite(led01, HIGH);
    
}
