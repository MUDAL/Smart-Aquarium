#include <WiFi.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include <WiFiManager.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h> //Version 1.1.2
#include "MNI.h"

//Maximum number of characters for HiveMQ topic(s)
#define SIZE_TOPIC             30
//Define textbox for MQTT publish topic
WiFiManagerParameter subTopic("0","HiveMQ Subscribe topic","",SIZE_TOPIC);
Preferences preferences; //for accessing ESP32 flash memory

//Type(s)
typedef struct
{
  float ph;
  float temperature;
  uint16_t tds;
  uint16_t turbidity;  
}sensor_t;

//RTOS Handle(s)
TaskHandle_t wifiTaskHandle;
QueueHandle_t nodeToAppQueue;

/**
 * @brief Store new data to specified location in ESP32's flash memory 
 * if the new is different from the old.  
*/
static void StoreNewFlashData(const char* flashLoc,const char* newData,
                              const char* oldData,uint8_t dataSize)
{
  if(strcmp(newData,"") && strcmp(newData,oldData))
  {
    preferences.putBytes(flashLoc,newData,dataSize);
  }
}

void setup() 
{
  setCpuFrequencyMhz(80);
  Serial.begin(115200);
  preferences.begin("S-Aqu",false);
  nodeToAppQueue = xQueueCreate(1,sizeof(sensor_t));
  if(nodeToAppQueue != NULL)
  {
    Serial.println("Node-Application Queue successfully created");
  }
  else
  {
    Serial.println("Node-Application Queue failed");
  }
  xTaskCreatePinnedToCore(WiFiManagementTask,"",7000,NULL,1,&wifiTaskHandle,1);
  xTaskCreatePinnedToCore(ApplicationTask,"",30000,NULL,1,NULL,1);
  xTaskCreatePinnedToCore(NodeTask,"",25000,NULL,1,NULL,1);
}

void loop() 
{
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
  wm.addParameter(&subTopic);
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
  static LiquidCrystal_I2C lcd(0x27,16,2);
  static sensor_t sensorData;
  bool isWifiTaskSuspended = false;
  //Startup message
  lcd.init();
  lcd.backlight();
  lcd.print(" SMART AQUARIUM");
  vTaskDelay(pdMS_TO_TICKS(1500)); 
  lcd.clear();
  lcd.print("STATUS: ");
  lcd.setCursor(0,1);
  lcd.print("LOADING...");
  vTaskDelay(pdMS_TO_TICKS(1500)); 
  lcd.clear();
  //Simple FSM to periodically change parameters being displayed.
  const uint8_t displayState1 = 0;
  const uint8_t displayState2 = 1;
  uint8_t displayState = displayState1; 
  uint32_t prevTime = millis(); 
   
  while(1)
  {
    //Suspend WiFi Management task if the system is already.... 
    //connected to a Wi-Fi network
    if(WiFi.status() == WL_CONNECTED && !isWifiTaskSuspended)
    {
      Serial.println("WIFI TASK: SUSPENDED");
      vTaskSuspend(wifiTaskHandle);
      isWifiTaskSuspended = true;
    }
    else if(WiFi.status() != WL_CONNECTED && isWifiTaskSuspended)
    {
      Serial.println("WIFI TASK: RESUMED");
      vTaskResume(wifiTaskHandle);
      isWifiTaskSuspended = false;
    }
    //Receive sensor data from the Node-Application Queue.
    if(xQueueReceive(nodeToAppQueue,&sensorData,0) == pdPASS)
    {
      Serial.println("--Data successfully received from Node task\n");
    }
    //FSM [Displays the received sensor data on the LCD]
    switch(displayState)
    {
      case displayState1: //Display PH and Temperature
        lcd.setCursor(0,0);
        lcd.print("PH: ");
        lcd.print(sensorData.ph,1);
        lcd.setCursor(0,1);
        lcd.print("TEMP: ");
        lcd.print(sensorData.temperature,2);
        lcd.print("C    ");
        if((millis() - prevTime) >= 2000)
        {
          displayState = displayState2;
          prevTime = millis();
          lcd.clear();
        }
        break;
      
      case displayState2: //Display TDS and Turbidity
        lcd.setCursor(0,0);
        lcd.print("TDS: ");
        lcd.print(sensorData.tds);
        lcd.print("ppm    ");
        lcd.setCursor(0,1);
        lcd.print("TURB: ");
        lcd.print(sensorData.turbidity);
        lcd.print("NTU    ");
        if((millis() - prevTime) >= 2000)
        {
          displayState = displayState1;
          prevTime = millis();
          lcd.clear();
        }
        break;
    }
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
    if((millis() - prevTime) >= 2500)
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
        Serial.println("--Received serial data from node\n");
        sensorData.ph = mni.DecodeData(MNI::RxDataId::PH) / 10.0; 
        sensorData.temperature = mni.DecodeData(MNI::RxDataId::TEMPERATURE) / 100.0; 
        sensorData.tds = mni.DecodeData(MNI::RxDataId::TDS);
        sensorData.turbidity = mni.DecodeData(MNI::RxDataId::TURBIDITY);
        //Debug
        Serial.print("PH: ");
        Serial.println(sensorData.ph,1); 
        Serial.print("Temperature: ");
        Serial.print(sensorData.temperature,2); 
        Serial.println(" C");
        Serial.print("TDS: ");
        Serial.print(sensorData.tds); 
        Serial.println(" ppm");
        Serial.print("Turbidity: ");
        Serial.print(sensorData.turbidity); 
        Serial.println(" NTU\n"); 
        //Place sensor data in the Node-Application Queue
        if(xQueueSend(nodeToAppQueue,&sensorData,0) == pdPASS)
        {
          Serial.println("--Data successfully sent to Application task\n");
        }
        else
        {
          Serial.println("--Failed to send data to Application task\n");
        }
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
  char prevSubTopic[SIZE_TOPIC] = {0};
  preferences.getBytes("0",prevSubTopic,SIZE_TOPIC);
  StoreNewFlashData("0",subTopic.getValue(),prevSubTopic,SIZE_TOPIC);
}
