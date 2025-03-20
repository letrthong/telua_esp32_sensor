// EmonLibrary examples openenergymonitor.org, Licence GNU GPL V3
#include <Wire.h>
#include "EmonLib.h"                   // Include Emon Library
#include <driver/adc.h>


#include <Arduino.h>
EnergyMonitor emon1;                   // Create an instance

#define ADC_INPUT 34
float voltage=220.00;
float pf=0.95;  
#define ADC_BITS    10 
#define ADC_COUNTS  (1<<ADC_BITS)
 
 double amps;
unsigned long lastMeasurement = 0;
unsigned long timeFinishedSetup = 0;
int counter = 0;
void setup()
{  
  Serial.begin(115200);
 // adc1_config_channel_atten(ADC1_CHANNEL_6, ADC_ATTEN_DB_11);
  analogReadResolution(ADC_BITS);    // 12 bit ADC

  emon1.current(ADC_INPUT, 11);            // Current: input pin, calibration.
}

//https://simplyexplained.com/blog/Home-Energy-Monitor-ESP32-CT-Sensor-Emonlib/
void loop()
{ 
   unsigned long currentMillis = millis();
 if(currentMillis - lastMeasurement > 1000)
  {   
       //emon.calcVI(20,2000);  
       //  current = emon.Irms;     

      amps = emon1.calcIrms(1480);  
 
       lastMeasurement = millis();
  }else{
      
      counter = counter +1;
      if(counter > 10){
         counter = 0;
        Serial.print("amps: ");
        Serial.print(amps);
        Serial.println(" A");

        // Serial.print("Power: ");
        // Serial.print(power);
        // Serial.println(" Watts");  
      }
    
    delay(100);

  }
     

}