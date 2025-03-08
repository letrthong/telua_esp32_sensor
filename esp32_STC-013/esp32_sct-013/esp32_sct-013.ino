// https://lastminuteengineers.com/esp32-basics-adc/
// Potentiometer is connected to GPIO 34 (Analog ADC1_CH6) 
// https://docs.espressif.com/projects/arduino-esp32/en/latest/api/adc.html
//https://github.com/espressif/arduino-esp32/issues/8999
const int potPin_adc = 34;

// variable for storing the potentiometer value
 
int count = 0 ;
// 2^12
int  volt_input = 3.3;

//set the resolution to the 13 bits (0-8192)  
int max_value =  4096;
void setup() {
  
  Serial.begin(115200);
  delay(1000);
  // set the resolution to the 13 bits (0-8192)  --> this is default, but just to make sure
   analogReadResolution(12);
   Serial.println("ADC1_CH6");
}

void loop() {
  //get ADC value for a given pin/ADC channel in millivolts.
  int analogVolts = analogReadMilliVolts(potPin_adc);
  if(analogVolts > 150){
    Serial.printf("12 bit - ADC millivolts value = %d\n",analogVolts);
    count = 0;

    float currentValue = (analogVolts*100)/3300;
    Serial.println(currentValue);
  }
  else
  {
      if(count > 20){
          count = 0;
           Serial.println("ADC1_CH6");
      }
      
  }


  count = count +1;

  
  delay(100);
}
