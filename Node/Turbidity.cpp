#include <Arduino.h>
#include "Turbidity.h"

Turbidity::Turbidity(uint8_t sensorPin,uint8_t numOfSamples)
{
  this->sensorPin = sensorPin;
  this->numOfSamples = numOfSamples;
}

float Turbidity::GetValue(void)
{
  float avgRawData = 0;
  for(int i = 0; i < numOfSamples; i++)
  {
    avgRawData += analogRead(this->sensorPin);  
  }
  avgRawData = avgRawData / (float) this->numOfSamples;
  float volt = (avgRawData * 5.0) / 1024.0;  
  float turbidity = lround((-1120.4*volt*volt + 5742.3*volt - 4352.9) * 10.0) / 10.0; //round to 1dp
  if(lround(turbidity) > 3000)
  {
    turbidity = 3000;
  }
  else if(lround(turbidity) < 0)
  {
    turbidity = 0;
  }
  return turbidity;
}
