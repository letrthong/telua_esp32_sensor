// https://lastminuteengineers.com/esp32-basics-adc/
// Potentiometer is connected to GPIO 34 (Analog ADC1_CH6) 
const int potPin = 34;

// variable for storing the potentiometer value
int potPin_adc = 0;
int count = 0 ;
// 2^12
int  volt_input = 3.3;
int max_value =  4096;
void setup() {
  
  Serial.begin(115200);
  delay(1000);
   Serial.println("ADC1_CH6");
}

void loop() {
  //get ADC value for a given pin/ADC channel in millivolts.
  int potValue = analogReadMilliVolts(potPin_adc);
  if(potValue > 150){
    Serial.println(potValue);
    count = 0;
    float currentValue = (potValue*100)/3300;
    Serial.println(currentValue);
  }
  else
  {
      if(count > 150){
          count = count;
           Serial.println("ADC1_CH6");
      }
      
  }


  count = count +1;

  
  delay(100);
}
