#pragma once

class PH
{
  private:
    uint8_t sensorPin;
    uint8_t numOfSamples;
    float calibrationValue;
  public:
    PH(uint8_t sensorPin,float calibrationValue,uint8_t numOfSamples = 100);
    float GetValue(void);
};
