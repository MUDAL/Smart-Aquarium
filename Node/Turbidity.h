#pragma once

class Turbidity
{
  private:
    uint8_t sensorPin;
    
  public:
    Turbidity(uint8_t sensorPin);
    float GetValue(void);
};
