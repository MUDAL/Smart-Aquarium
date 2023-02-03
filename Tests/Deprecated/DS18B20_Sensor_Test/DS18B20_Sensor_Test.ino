#include <OneWire.h> //2.3.7
#include <DallasTemperature.h> //3.9.0

const uint8_t ds18b20Pin = 2;

OneWire oneWire(ds18b20Pin);
DallasTemperature ds18b20(&oneWire);
float temperature = 0;

void setup(void)
{
  Serial.begin(9600);
  ds18b20.begin();
}

void loop(void)
{ 
  ds18b20.requestTemperatures(); 
  temperature = ds18b20.getTempCByIndex(0);
  Serial.println(temperature);
  delay(500);
}
