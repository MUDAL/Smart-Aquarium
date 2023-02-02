#include <Arduino.h>
#include "Turbidity.h"

Turbidity::Turbidity(uint8_t sensorPin,uint8_t numOfSamples)
{
  this->sensorPin = sensorPin;
  this->numOfSamples = numOfSamples;
}

int Turbidity::GetValue(void)
{
  float avgRawData = 0;
  for(int i = 0; i < numOfSamples; i++)
  {
    avgRawData += analogRead(this->sensorPin);  
  }
  avgRawData = avgRawData / (float) this->numOfSamples;
  float volt = (avgRawData * 5.0) / 1024.0;  
  int turbidity = lround(-1120.4*volt*volt + 5742.3*volt - 4352.9);
  if(turbidity > 3000)
  {
    turbidity = 3000;
  }
  else if(turbidity < 0)
  {
    turbidity = 0;
  }
  return turbidity;
}
