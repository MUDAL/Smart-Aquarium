const float calibrationValue = 21.34 - 0.75;
const uint8_t numOfSamples = 100;

void setup() 
{
  // put your setup code here, to run once:
  Serial.begin(9600);
}

void loop() 
{
  // put your main code here, to run repeatedly:
  float avgRawData = 0;
  for(int i = 0; i < numOfSamples; i++)
  {
    avgRawData += analogRead(A0);  
  }
  avgRawData = avgRawData / (float) numOfSamples;
  float volt = (avgRawData * 5.0) / 1024.0;
  float ph = -5.70 * volt + calibrationValue;
  Serial.println(ph);
  delay(500);
}
