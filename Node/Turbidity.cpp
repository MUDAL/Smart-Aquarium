#include <Arduino.h>
#include "Turbidity.h"

Turbidity::Turbidity(uint8_t sensorPin,uint8_t numOfSamples)
{
  this->sensorPin = sensorPin;
  this->numOfSamples = numOfSamples;
}

float Turbidity::GetOutputVoltage(void)
{
  float avgRawData = 0;
  for(int i = 0; i < numOfSamples; i++)
  {
    avgRawData += analogRead(this->sensorPin);  
  }
  avgRawData = avgRawData / (float) this->numOfSamples;
  float turbidityInVolts = (avgRawData * 5.0) / 1024.0;  
  return turbidityInVolts;
}
