#include <OneWire.h> //2.3.7
#include <DallasTemperature.h> //3.9.0
#include "GravityTDS.h"

//Algorithm: Read PH(first),temperature(second),TDS(last) and .....
//repeat the sequence every 2s.

//PH sensor
const uint8_t phSensorPin = A0;
const float calibrationValue = 21.34 - 0.75;
const uint8_t numOfSamples = 100;
//TDS sensor
const uint8_t tdsPin = A1;
const uint8_t tdsVccControlPin = 3;
const uint8_t tdsGndControlPin = 4;
GravityTDS gravityTds;
//Temperature sensor
const uint8_t ds18b20Pin = 2;
OneWire oneWire(ds18b20Pin);
DallasTemperature ds18b20(&oneWire);
float temperature = 0;

void setup() 
{
  // put your setup code here, to run once:
  Serial.begin(9600);
  ds18b20.begin();
  pinMode(tdsVccControlPin,OUTPUT);
  pinMode(tdsGndControlPin,OUTPUT);
  gravityTds.setPin(tdsPin);
  gravityTds.setAref(5.0);  //reference voltage on ADC, default 5.0V on Arduino UNO
  gravityTds.setAdcRange(1024);  //1024 for 10bit ADC;4096 for 12bit ADC
  gravityTds.begin();  //initialization  
}

void loop() 
{
  // put your main code here, to run repeatedly:
  //Get PH 
  float avgRawData = 0;
  for(int i = 0; i < numOfSamples; i++)
  {
    avgRawData += analogRead(phSensorPin);  
  }
  avgRawData = avgRawData / (float) numOfSamples;
  float volt = (avgRawData * 5.0) / 1024.0;
  float ph = -5.70 * volt + calibrationValue;
  Serial.print("PH: ");
  Serial.println(ph);   
  //Switch TDS sensor ON
  digitalWrite(tdsVccControlPin,HIGH);
  digitalWrite(tdsGndControlPin,HIGH);
  ds18b20.requestTemperatures(); 
  temperature = ds18b20.getTempCByIndex(0);
  Serial.print("Temperature = ");
  Serial.print(temperature);
  Serial.println("C");
  gravityTds.setTemperature(temperature);  // set the temperature and execute temperature compensation
  gravityTds.update();  //sample and calculate 
  float tdsValue = gravityTds.getTdsValue();  // then get the value
  Serial.print("TDS = ");
  Serial.print(tdsValue,0);
  Serial.println("ppm");  
  //Switch TDS sensor OFF
  digitalWrite(tdsVccControlPin,LOW);
  digitalWrite(tdsGndControlPin,LOW);
  delay(2000);  
}
