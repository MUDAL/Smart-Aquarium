#include <WiFi.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include <WiFiManager.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h> //Version 1.1.2
#include "MNI.h"

//Type(s)
typedef struct
{
  float ph;
  float temperature;
  uint16_t tds;
  uint16_t turbidity;  
}sensor_t;

//Task handle(s)
TaskHandle_t wifiTaskHandle;

void setup() 
{
  // put your setup code here, to run once:
  setCpuFrequencyMhz(80);
  Serial.begin(115200);
  xTaskCreatePinnedToCore(WiFiManagementTask,"",7000,NULL,1,&wifiTaskHandle,1);
  xTaskCreatePinnedToCore(ApplicationTask,"",30000,NULL,1,NULL,1);
  xTaskCreatePinnedToCore(NodeTask,"",25000,NULL,1,NULL,1);
}

void loop() 
{
  // put your main code here, to run repeatedly:

}

/**
 * @brief Manages WiFi configurations (STA and AP modes). Connects
 * to an existing/saved network if available, otherwise it acts as
 * an AP in order to receive new network credentials.
*/
void WiFiManagementTask(void* pvParameters)
{
  const uint16_t accessPointTimeout = 50000; //millisecs
  static WiFiManager wm;
  WiFi.mode(WIFI_STA);  
  /* ADD WiFi manager parameters if any */
  wm.setConfigPortalBlocking(false);
  wm.setSaveParamsCallback(WiFiManagerCallback);   
  //Auto-connect to previous network if available.
  //If connection fails, ESP32 goes from being a station to being an access point.
  Serial.print(wm.autoConnect("AQUARIUM")); 
  Serial.println("-->WiFi status");   
  bool accessPointMode = false;
  uint32_t startTime = 0;    
  
  while(1)
  {
    wm.process();
    if(WiFi.status() != WL_CONNECTED)
    {
      if(!accessPointMode)
      {
        if(!wm.getConfigPortalActive())
        {
          wm.autoConnect("AQUARIUM"); 
        }
        accessPointMode = true; 
        startTime = millis(); 
      }
      else
      {
        //reset after a timeframe (device shouldn't spend too long as an access point)
        if((millis() - startTime) >= accessPointTimeout)
        {
          Serial.println("\nAP timeout reached, system will restart for better connection");
          vTaskDelay(pdMS_TO_TICKS(1000));
          esp_restart();
        }
      }
    }
    else
    {
      if(accessPointMode)
      {   
        accessPointMode = false;
        Serial.println("Successfully connected, system will restart now");
        vTaskDelay(pdMS_TO_TICKS(1000));
        esp_restart();
      }
    }    
  }
}

void ApplicationTask(void* pvParameters)
{
  static LiquidCrystal_I2C lcd(0x27,20,4);
  while(1)
  {
    
  }
}

/**
 * @brief Handles communication between the master and node.
*/
void NodeTask(void* pvParameters)
{
  static MNI mni(&Serial2); //MNI: Master-Node-Interface
  static sensor_t sensorData;
  uint32_t prevTime = millis();
  while(1)
  {
    //Request for sensor data from the node (periodically)
    if((millis() - prevTime) >= 3000)
    {
      mni.EncodeData(MNI::QUERY,MNI::TxDataId::DATA_QUERY);
      mni.TransmitData();
      prevTime = millis();
    }
    //Decode data received from node
    if(mni.ReceivedData())
    {
      if(mni.DecodeData(MNI::RxDataId::DATA_ACK) == MNI::ACK)
      {
        //Divide received PH and temperature data by 10 and 100 ...
        //respectively in order to get the actual readings.
        Serial.println("Received data from node");
        sensorData.ph = mni.DecodeData(MNI::RxDataId::PH) / 10.0; 
        sensorData.temperature = mni.DecodeData(MNI::RxDataId::TEMPERATURE) / 100.0; 
        sensorData.tds = mni.DecodeData(MNI::RxDataId::TDS);
        sensorData.turbidity = mni.DecodeData(MNI::RxDataId::TURBIDITY);
        //Debug
        Serial.print("PH: ");
        Serial.println(sensorData.ph,1); 
        Serial.print("Temperature: ");
        Serial.print(sensorData.temperature,2); 
        Serial.println("C");
        Serial.print("TDS: ");
        Serial.print(sensorData.tds); 
        Serial.println("ppm");
        Serial.print("Turbidity: ");
        Serial.print(sensorData.turbidity); 
        Serial.println("NTU\n"); 
      }
    }
  }
}

/**
 * @brief Callback function that is called whenever WiFi
 * manager parameters are received.
*/
void WiFiManagerCallback(void) 
{

}
