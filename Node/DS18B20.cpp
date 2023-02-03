#include <Arduino.h>
#include "DS18B20.h"

DS18B20::DS18B20(uint8_t oneWirePin)
{
  oneWire.begin(oneWirePin);
  temperatureSensor.setOneWire(&oneWire);
  temperatureSensor.begin();
}

float DS18B20::GetValue(void)
{
  temperatureSensor.requestTemperatures(); 
  float temperature = temperatureSensor.getTempCByIndex(0);  
  return temperature;
}
