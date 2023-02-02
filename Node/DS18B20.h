#pragma once

#include <OneWire.h> //2.3.7
#include <DallasTemperature.h> //3.9.0

class DS18B20
{
  private:
    OneWire oneWire;
    DallasTemperature temperatureSensor;
    
  public:
    DS18B20(uint8_t oneWirePin);
    float GetValue(void);
};
