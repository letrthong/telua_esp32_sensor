#include <Adafruit_MCP4725.h>

 
Adafruit_MCP4725 MCP4725;

void setup(void) 
{
 Serial.begin(115200);
  bool ret = MCP4725.begin(0x62); // The I2C Address of my module 
  if(ret == false){
    Serial.println("  Can not detect MCP4725 ");
  }
}

void loop(void) 
{
 uint32_t MCP4725_value;
 int adcInput = 0;
 float voltageIn = 0; 
 float MCP4725_reading;
 //3.3 is your supply voltage
 float max_Voltage = 1.0;
 for (MCP4725_value = 0; MCP4725_value < 4096; MCP4725_value = MCP4725_value + 128)
 {
    MCP4725_reading = (max_Voltage/4096.0) * MCP4725_value; 
    bool ret = MCP4725.setVoltage(MCP4725_value, false);
    if(ret == false){
      Serial.print("\t Can not write MCP4725");
    }
    delay(5000);
    Serial.println("");
    Serial.print("Expected Voltage: ");
    Serial.print(MCP4725_reading,max_Voltage);
     
 } 
}