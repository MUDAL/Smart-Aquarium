#include <SoftwareSerial.h>
#include "MNI.h"
#include "PH_Sensor.h"
#include "DS18B20.h"
#include "TDS.h"
#include "Turbidity.h"

//Algorithm: Read PH(first),temperature(second),TDS(last) and .....
//repeat the sequence periodically.

namespace Pin
{
  const uint8_t phSensor = A0;
  const uint8_t temperatureSensor = 2;
  const uint8_t tdsSensor = A1;
  const uint8_t tdsVcc = 3;
  const uint8_t tdsGnd = 4;
  const uint8_t turbiditySensor = A2;
};

const float phCalibration = 21.34 - 0.75;
const uint8_t numberOfSensorSamples = 100;

PH phSensor(Pin::phSensor,phCalibration,numberOfSensorSamples);
DS18B20 temperatureSensor(Pin::temperatureSensor);
TDS tdsSensor(Pin::tdsSensor,Pin::tdsVcc,Pin::tdsGnd);

void setup() 
{
  Serial.begin(9600);
}

void loop() 
{
  float ph = phSensor.GetValue();
  Serial.print("PH: ");
  Serial.println(ph);   

  float temperature = temperatureSensor.GetValue();
  Serial.print("Temperature = ");
  Serial.print(temperature);
  Serial.println("C");
  float tdsValue = tdsSensor.GetValue(temperature);

  Serial.print("TDS = ");
  Serial.print(tdsValue,0);
  Serial.println("ppm");  

  delay(2000);  
}
