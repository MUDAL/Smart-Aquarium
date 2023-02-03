#pragma once

class Turbidity
{
  private:
    uint8_t sensorPin;
    uint8_t numOfSamples;
    
  public:
    Turbidity(uint8_t sensorPin,uint8_t numOfSamples);
    int GetValue(void);
};
