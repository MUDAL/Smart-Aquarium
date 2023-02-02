#include <Arduino.h>
#include "Turbidity.h"

Turbidity::Turbidity(uint8_t sensorPin)
{
  this->sensorPin = sensorPin;
}

float Turbidity::GetValue(void)
{
  float turbidity = 0;
  return turbidity;
}
