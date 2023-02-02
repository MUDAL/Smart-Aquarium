#pragma once

#include "GravityTDS.h"

class TDS
{
  private:
    uint8_t signalPin;
    uint8_t vccControlPin;
    uint8_t gndControlPin;
    int startupTime;
    GravityTDS gravityTds;
    
  public:
    TDS(uint8_t signalPin,uint8_t vccControlPin,uint8_t gndControlPin,
        float refVoltage = 5.0,int adcResolution = 1024);
    float GetValue(float temperature);
};
