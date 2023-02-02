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
  const uint8_t nodeRx = 6;
  const uint8_t nodeTx = 7;
};

const uint8_t numberOfSensorSamples = 100;
const float phCalibration = 21.34 - 0.75;
//Sensors
PH phSensor(Pin::phSensor,phCalibration,numberOfSensorSamples);
DS18B20 temperatureSensor(Pin::temperatureSensor);
TDS tdsSensor(Pin::tdsSensor,Pin::tdsVcc,Pin::tdsGnd);
Turbidity turbiditySensor(Pin::turbiditySensor,numberOfSensorSamples);
//Master-Node-Interface 
SoftwareSerial nodeSerial(Pin::nodeRx,Pin::nodeTx);
MNI mni(&nodeSerial);

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

  int turbidity = turbiditySensor.GetValue();
  Serial.print("Turbidity = ");
  Serial.print(turbidity);
  Serial.println("NTU");
   
  int tdsValue = tdsSensor.GetValue(temperature);
  Serial.print("TDS = ");
  Serial.print(tdsValue);
  Serial.println("ppm\n");  

  delay(2000);  
}
