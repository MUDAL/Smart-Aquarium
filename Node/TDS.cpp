#include <Arduino.h>
#include "TDS.h"

TDS::TDS(uint8_t signalPin,uint8_t vccControlPin,uint8_t gndControlPin,
         float refVoltage,int adcResolution)
{
  this->signalPin = signalPin;
  this->vccControlPin = vccControlPin;
  this->gndControlPin = gndControlPin;
  pinMode(this->vccControlPin,OUTPUT);
  pinMode(this->gndControlPin,OUTPUT);
  digitalWrite(this->vccControlPin,LOW);
  digitalWrite(this->gndControlPin,LOW);
  gravityTds.setPin(this->signalPin);
  gravityTds.setAref(refVoltage);  //reference voltage on ADC, default 5.0V on Arduino UNO
  gravityTds.setAdcRange(adcResolution);  //1024 for 10bit ADC;4096 for 12bit ADC
  gravityTds.begin();  //initialization    
}

float TDS::GetValue(float temperature)
{
  //Turn TDS sensor on, get readings, then turn it off
  digitalWrite(this->vccControlPin,HIGH);
  digitalWrite(this->gndControlPin,HIGH);  
  delay(500); //start-up time
  gravityTds.setTemperature(temperature);  // set the temperature and execute temperature compensation
  gravityTds.update();  //sample and calculate 
  float tdsValue = gravityTds.getTdsValue(); 
  digitalWrite(this->vccControlPin,LOW);
  digitalWrite(this->gndControlPin,LOW);
  return tdsValue;
}
