#include <OneWire.h> //2.3.7
#include <DallasTemperature.h> //3.9.0

const uint8_t numOfSamples = 100;
int tdsArray[numOfSamples];
const uint8_t tdsVccControlPin = 3;
const uint8_t tdsGndControlPin = 4;
const uint8_t ds18b20Pin = 2;
OneWire oneWire(ds18b20Pin);
DallasTemperature ds18b20(&oneWire);
float temperature = 0;

void Swap(int* num1,int* num2)
{
  int temp = *num1;
  *num1 = *num2;
  *num2 = temp;
}

void Sort(int arr[],int len)
{
  int i, j;
  for (i = 0; i < len - 1; i++)
  {
    for (j = 0; j < len - i - 1; j++)
    {
      if (arr[j] > arr[j + 1])
      {
        Swap(&arr[j],&arr[j + 1]);
      }
    }
  }
}

int GetMedian(int arr[],int len)
{
  int midIndex = 0;
  int median = 0;
  Sort(arr,len);
  if(len % 2 == 0)
  {
    midIndex = len / 2;
    median = (arr[midIndex] + arr[midIndex-1]) / 2;
  }
  else
  {
    midIndex = len / 2;
    median = arr[midIndex];
  }
  return median;
}

void setup() 
{
  // put your setup code here, to run once:
  Serial.begin(9600);
  ds18b20.begin();
  pinMode(tdsVccControlPin,OUTPUT);
  pinMode(tdsGndControlPin,OUTPUT);
}

void loop() 
{
  // put your main code here, to run repeatedly:
  digitalWrite(tdsVccControlPin,HIGH);
  digitalWrite(tdsGndControlPin,HIGH);
  Serial.print("Samples: ");
  for(int i = 0; i < numOfSamples; i++)
  {
    Serial.print(i);
    Serial.print("- ");
    tdsArray[i] = analogRead(A1);
    Serial.println(tdsArray[i]);
  }
  Serial.print("tds raw data (median) = ");
  int tdsRawData = GetMedian(tdsArray,numOfSamples);
  Serial.println(tdsRawData);  
  float volt = (tdsRawData * 5.0) / 1024.0;
  
  ds18b20.requestTemperatures(); 
  temperature = ds18b20.getTempCByIndex(0);
  float compensationCoefficient = 1.0 + 0.02 * (temperature - 25.0);
  float compensationVoltage = volt / compensationCoefficient;
  float tds = (133.42*volt*volt*volt - 255.86*volt*volt + 857.39*volt)*0.5;
  
  Serial.print("Temperature = ");
  Serial.println(temperature);
  Serial.print("TDS = ");
  Serial.println(tds);
  delay(500);

  digitalWrite(tdsVccControlPin,LOW);
  digitalWrite(tdsGndControlPin,LOW);
  delay(2000);
}
