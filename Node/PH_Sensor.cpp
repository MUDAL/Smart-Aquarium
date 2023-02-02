#include <Arduino.h>
#include "PH_Sensor.h"

PH::PH(uint8_t sensorPin,float calibrationValue,uint8_t numOfSamples)
{
  this->sensorPin = sensorPin;
  this->numOfSamples = numOfSamples;
  this->calibrationValue = calibrationValue;
}

void PH::Calibrate(float calibrationValue)
{
  this->calibrationValue = calibrationValue;
}

float PH::GetValue(void)
{
  float avgRawData = 0;
  for(int i = 0; i < numOfSamples; i++)
  {
    avgRawData += analogRead(this->sensorPin);  
  }
  avgRawData = avgRawData / (float) this->numOfSamples;
  float volt = (avgRawData * 5.0) / 1024.0;
  float ph = -5.70 * volt + this->calibrationValue;
  return ph;  
}
