#include <SoftwareSerial.h>
#include "MNI.h"
#include "PH_Sensor.h"
#include "DS18B20.h"
#include "TDS.h"
#include "Turbidity.h"

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
  if(mni.ReceivedData())
  {
    if(mni.DecodeData(MNI::RxDataId::DATA_QUERY) == MNI::QUERY)
    {
      float ph = phSensor.GetValue();
      float temperature = temperatureSensor.GetValue();
      uint16_t tds = tdsSensor.GetValue(temperature);
      float turbidityInVolts = turbiditySensor.GetOutputVoltage();

      //Debug
      Serial.print("PH: ");
      Serial.println(ph,1);   
      Serial.print("Temperature = ");
      Serial.print(temperature);
      Serial.println(" C");
      Serial.print("TDS = ");
      Serial.print(tds);
      Serial.println(" ppm");
      Serial.print("Turbidity = ");
      Serial.print(turbidityInVolts,2);
      Serial.println(" [V]\n"); 
      
      mni.EncodeData(MNI::ACK,MNI::TxDataId::DATA_ACK);
      mni.EncodeData((ph * 10),MNI::TxDataId::PH);
      mni.EncodeData((temperature * 100),MNI::TxDataId::TEMPERATURE);
      mni.EncodeData(tds,MNI::TxDataId::TDS);
      mni.EncodeData((turbidityInVolts * 100),MNI::TxDataId::TURBIDITY);
      mni.TransmitData();
    }
  }
}
