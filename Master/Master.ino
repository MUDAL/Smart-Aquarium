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
WiFiManagerParameter subTopic("0","HiveMQ Subscription topic","",SIZE_TOPIC);
Preferences preferences; //for accessing ESP32 flash memory

//Type(s)
typedef struct
{
  float ph;
  float temperature;
  uint16_t tds;
  float turbidity;  
}sensor_t;

//RTOS Handle(s)
TaskHandle_t wifiTaskHandle;
TaskHandle_t nodeTaskHandle;
QueueHandle_t nodeToAppQueue;
QueueHandle_t nodeToMqttQueue;

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
  nodeToMqttQueue = xQueueCreate(1,sizeof(sensor_t));
  if(nodeToAppQueue != NULL)
  {
    Serial.println("Node-Application Queue successfully created");
  }
  else
  {
    Serial.println("Node-Application Queue failed");
  }
  if(nodeToMqttQueue != NULL)
  {
    Serial.println("Node-MQTT Queue successfully created");
  }
  else
  {
    Serial.println("Node-MQTT Queue failed");
  }  
  xTaskCreatePinnedToCore(WiFiManagementTask,"",7000,NULL,1,&wifiTaskHandle,1);
  xTaskCreatePinnedToCore(ApplicationTask,"",30000,NULL,1,NULL,1);
  xTaskCreatePinnedToCore(NodeTask,"",25000,NULL,1,&nodeTaskHandle,1);
  xTaskCreatePinnedToCore(MqttTask,"",7000,NULL,1,NULL,1);
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

/**
 * @brief Handles main application logic.
*/
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
  vTaskResume(nodeTaskHandle);
  
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
      Serial.println("--Application task received data from Node task\n");
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
        if((millis() - prevTime) >= 5000)
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
        lcd.print(sensorData.turbidity,1);
        lcd.print("NTU    ");
        if((millis() - prevTime) >= 5000)
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
 * @brief Handles communication between the master and node via a
 * serial interface (i.e. MNI).
 * NB: MNI stands for 'Master-Node-Interface'
*/
void NodeTask(void* pvParameters)
{
  vTaskSuspend(NULL);
  static MNI mni(&Serial2); 
  static sensor_t sensorData;
  //Initial request for sensor data from the node
  mni.EncodeData(MNI::QUERY,MNI::TxDataId::DATA_QUERY);
  mni.TransmitData();  
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
        //Divide received PH,temperature,and turbidity data by 10,100,and 10 ...
        //respectively in order to get the actual readings.
        Serial.println("--Received serial data from node\n");
        sensorData.ph = mni.DecodeData(MNI::RxDataId::PH) / 10.0; 
        sensorData.temperature = mni.DecodeData(MNI::RxDataId::TEMPERATURE) / 100.0; 
        sensorData.tds = mni.DecodeData(MNI::RxDataId::TDS);
        sensorData.turbidity = mni.DecodeData(MNI::RxDataId::TURBIDITY) / 10.0;
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
        Serial.print(sensorData.turbidity,1); 
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
        //Place sensor data in the Node-MQTT Queue
        if(xQueueSend(nodeToMqttQueue,&sensorData,0) == pdPASS)
        {
          Serial.println("--Data successfully sent to MQTT task\n");
        }
        else
        { //Can occur if the ESP32 is not connected to a WiFi hotspot.
          Serial.println("--Failed to send data to MQTT task\n");
        }        
      }
    }
  }
}

/**
 * @brief Handles uploading of sensor data to the cloud via MQTT.
*/
void MqttTask(void* pvParameters)
{
  static sensor_t sensorData;
  static WiFiClient wifiClient;
  static PubSubClient mqttClient(wifiClient);
  char prevSubTopic[SIZE_TOPIC] = {0};
  const char *mqttBroker = "broker.hivemq.com";
  const uint16_t mqttPort = 1883;  
  
  while(1)
  {
    if(WiFi.status() == WL_CONNECTED)
    {       
      if(!mqttClient.connected())
      { 
        preferences.getBytes("0",prevSubTopic,SIZE_TOPIC);
        mqttClient.setServer(mqttBroker,mqttPort);
        while(!mqttClient.connected())
        {
          String clientID = String(WiFi.macAddress());
          if(mqttClient.connect(clientID.c_str()))
          {
            Serial.println("Connected to HiveMQ broker");
          }
        } 
      }
      else
      {
        //Receive sensor data from the Node-MQTT Queue.
        if(xQueueReceive(nodeToMqttQueue,&sensorData,0) == pdPASS)
        {
          Serial.println("--MQTT task received data from Node task\n");
          String dataToPublish = "PH: " + String(sensorData.ph,1) + " \n" +
                       "Temp:  " + String(sensorData.temperature,2) + " C\n" +
                       "TDS: " + String(sensorData.tds) + " ppm\n" + 
                       "Turbid: " + String(sensorData.turbidity,1) + " NTU";
          mqttClient.publish(prevSubTopic,dataToPublish.c_str());
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
