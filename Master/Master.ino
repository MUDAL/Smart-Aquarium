#include <WiFi.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include <WiFiManager.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h> //Version 1.1.2
#include "ThingSpeak.h" //Version 2.0.1
#include "MNI.h"
#include "numeric_lib.h"

//Maximum number of characters
#define SIZE_TOPIC             30
#define SIZE_CLIENT_ID         23
#define SIZE_CHANNEL_ID        30
#define SIZE_API_KEY           50
#define SIZE_THRESHOLD         20

//Textboxes
WiFiManagerParameter subTopic("0","HiveMQ Subscription topic","",SIZE_TOPIC);
WiFiManagerParameter clientID("A","MQTT client ID","",SIZE_CLIENT_ID);
WiFiManagerParameter channelId("2","ThingSpeak channel ID","",SIZE_CHANNEL_ID);
WiFiManagerParameter apiKey("3","ThingSpeak API key","",SIZE_API_KEY);
WiFiManagerParameter minPh("4","Minimum PH","",SIZE_THRESHOLD);
WiFiManagerParameter maxPh("5","Maximum PH","",SIZE_THRESHOLD);
WiFiManagerParameter minTemp("6","Minimum temperature","",SIZE_THRESHOLD);
WiFiManagerParameter maxTemp("7","Maximum temperature","",SIZE_THRESHOLD);
WiFiManagerParameter minTds("8","Minimum TDS","",SIZE_THRESHOLD);
WiFiManagerParameter maxTds("9","Maximum TDS","",SIZE_THRESHOLD);
Preferences preferences; //for accessing ESP32 flash memory

//Type(s)
typedef struct
{
  float ph;
  float temperature;
  uint16_t tds;
  float turbidity;  
}sensor_t;

typedef struct
{
  float minPh;
  float maxPh;
  float minTemp;
  float maxTemp;
  float minTds;
  float maxTds;
}limit_t;

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

/**
 * @brief Append str2 to str1 if 'condition' is true.
*/
static void AddStringIfTrue(char* str1,char* str2,bool condition)
{
  if(condition)
  {
    strcat(str1,str2);
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
  xTaskCreatePinnedToCore(ApplicationTask,"",50000,NULL,1,NULL,1);
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
  wm.addParameter(&clientID);
  wm.addParameter(&channelId);
  wm.addParameter(&apiKey);  
  wm.addParameter(&minPh);
  wm.addParameter(&maxPh);
  wm.addParameter(&minTemp);
  wm.addParameter(&maxTemp);
  wm.addParameter(&minTds);
  wm.addParameter(&maxTds);  
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
  static WiFiClient wifiClient;
  static LiquidCrystal_I2C lcd(0x27,16,2);
  static sensor_t sensorData;
  ThingSpeak.begin(wifiClient);
  bool isWifiTaskSuspended = false;
  //Previously stored data (in ESP32's flash)
  char prevChannelId[SIZE_CHANNEL_ID] = {0};
  char prevApiKey[SIZE_API_KEY] = {0};

  uint32_t prevConnectTime = millis();
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
    //Send data to ThingSpeak [periodically]
    if(WiFi.status() == WL_CONNECTED && ((millis() - prevConnectTime) >= 20000))
    {
      preferences.getBytes("2",prevChannelId,SIZE_CHANNEL_ID);
      preferences.getBytes("3",prevApiKey,SIZE_API_KEY);
      //Encode sensor data
      ThingSpeak.setField(1,sensorData.ph);
      ThingSpeak.setField(2,sensorData.temperature); 
      ThingSpeak.setField(3,sensorData.tds); 
      ThingSpeak.setField(4,sensorData.turbidity); 
      //Convert channel ID from string to integer
      uint32_t idInteger = 0;
      StringToInteger(prevChannelId,&idInteger); 
      int httpResponseCode = ThingSpeak.writeFields(idInteger,prevApiKey); //Send data to ThingSpeak  
      if(httpResponseCode == HTTP_CODE_OK)
      {
        lcd.clear();
        lcd.print("DATA UPLOAD: ");
        lcd.setCursor(0,1);
        lcd.print("SUCCESSFUL");
        vTaskDelay(pdMS_TO_TICKS(2000));
        lcd.clear();
      }
      prevConnectTime = millis();
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
        //Divide received PH,temperature,and turbidity data by 10,100,and 100 ...
        //respectively in order to get the actual readings.
        Serial.println("--Received serial data from node\n");
        sensorData.ph = mni.DecodeData(MNI::RxDataId::PH) / 10.0; 
        sensorData.temperature = mni.DecodeData(MNI::RxDataId::TEMPERATURE) / 100.0; 
        sensorData.tds = mni.DecodeData(MNI::RxDataId::TDS);
        sensorData.turbidity = mni.DecodeData(MNI::RxDataId::TURBIDITY) / 10.0;
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
  static char dataToPublish[400];
  
  char prevSubTopic[SIZE_TOPIC] = {0};
  char prevClientID[SIZE_CLIENT_ID] = {0};
  const char *mqttBroker = "broker.hivemq.com";
  const uint16_t mqttPort = 1883;  
  
  while(1)
  {
    if(WiFi.status() == WL_CONNECTED)
    {       
      if(!mqttClient.connected())
      { 
        memset(prevSubTopic,'\0',SIZE_TOPIC);
        memset(prevClientID,'\0',SIZE_CLIENT_ID);
        preferences.getBytes("0",prevSubTopic,SIZE_TOPIC);
        preferences.getBytes("A",prevClientID,SIZE_CLIENT_ID);
        mqttClient.setServer(mqttBroker,mqttPort);
        while(!mqttClient.connected())
        {
          if(mqttClient.connect(prevClientID))
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
          char prevMinPh[SIZE_THRESHOLD] = {0};
          char prevMaxPh[SIZE_THRESHOLD] = {0};
          char prevMinTemp[SIZE_THRESHOLD] = {0};
          char prevMaxTemp[SIZE_THRESHOLD] = {0};
          char prevMinTds[SIZE_THRESHOLD] = {0};
          char prevMaxTds[SIZE_THRESHOLD] = {0};     
               
          preferences.getBytes("4",prevMinPh,SIZE_THRESHOLD);
          preferences.getBytes("5",prevMaxPh,SIZE_THRESHOLD);
          preferences.getBytes("6",prevMinTemp,SIZE_THRESHOLD);
          preferences.getBytes("7",prevMaxTemp,SIZE_THRESHOLD);
          preferences.getBytes("8",prevMinTds,SIZE_THRESHOLD);
          preferences.getBytes("9",prevMaxTds,SIZE_THRESHOLD);

          strcat(dataToPublish,"Limits:\n");
          strcat(dataToPublish,"PH:");
          strcat(dataToPublish,prevMinPh);
          strcat(dataToPublish,"-");
          strcat(dataToPublish,prevMaxPh);
          strcat(dataToPublish,"\n");
          strcat(dataToPublish,"TEMP:");
          strcat(dataToPublish,prevMinTemp);
          strcat(dataToPublish,"-");
          strcat(dataToPublish,prevMaxTemp);
          strcat(dataToPublish,"C\n");
          strcat(dataToPublish,"TDS:");          
          strcat(dataToPublish,prevMinTds);
          strcat(dataToPublish,"-");
          strcat(dataToPublish,prevMaxTds);
          strcat(dataToPublish,"ppm\n\n");          

          limit_t sensorLim = {};
          StringToFloat(prevMinPh,&sensorLim.minPh);
          StringToFloat(prevMaxPh,&sensorLim.maxPh);
          StringToFloat(prevMinTemp,&sensorLim.minTemp);
          StringToFloat(prevMaxTemp,&sensorLim.maxTemp);
          StringToFloat(prevMinTds,&sensorLim.minTds);
          StringToFloat(prevMaxTds,&sensorLim.maxTds);

          bool isPhLow = lround(sensorData.ph * 10) < lround(sensorLim.minPh * 10);
          bool isPhHigh = lround(sensorData.ph * 10) > lround(sensorLim.maxPh * 10);
          bool isTempLow = lround(sensorData.temperature * 100) < lround(sensorLim.minTemp * 100);
          bool isTempHigh = lround(sensorData.temperature * 100) > lround(sensorLim.maxTemp * 100);
          bool isTdsLow = sensorData.tds < lround(sensorLim.minTds);
          bool isTdsHigh = sensorData.tds > lround(sensorLim.maxTds);

          strcat(dataToPublish,"Note:\n");
          AddStringIfTrue(dataToPublish,"LOW PH\n",isPhLow);
          AddStringIfTrue(dataToPublish,"HIGH PH\n",isPhHigh);
          AddStringIfTrue(dataToPublish,"LOW TEMP\n",isTempLow);
          AddStringIfTrue(dataToPublish,"HIGH TEMP\n",isTempHigh);
          AddStringIfTrue(dataToPublish,"LOW TDS\n",isTdsLow);
          AddStringIfTrue(dataToPublish,"HIGH TDS\n",isTdsHigh);

          if(isPhLow || isPhHigh || isTempLow || isTempHigh || isTdsLow || isTdsHigh)
          {
            mqttClient.publish(prevSubTopic,dataToPublish); 
          }
          uint32_t dataLen = strlen(dataToPublish);
          memset(dataToPublish,'\0',dataLen);
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
  char prevClientID[SIZE_CLIENT_ID] = {0};
  char prevChannelId[SIZE_CHANNEL_ID] = {0};
  char prevApiKey[SIZE_API_KEY] = {0}; 
  char prevMinPh[SIZE_THRESHOLD] = {0};
  char prevMaxPh[SIZE_THRESHOLD] = {0};
  char prevMinTemp[SIZE_THRESHOLD] = {0};
  char prevMaxTemp[SIZE_THRESHOLD] = {0};
  char prevMinTds[SIZE_THRESHOLD] = {0};
  char prevMaxTds[SIZE_THRESHOLD] = {0};
  preferences.getBytes("0",prevSubTopic,SIZE_TOPIC);
  preferences.getBytes("A",prevClientID,SIZE_CLIENT_ID);
  preferences.getBytes("2",prevChannelId,SIZE_CHANNEL_ID);
  preferences.getBytes("3",prevApiKey,SIZE_API_KEY);  
  preferences.getBytes("4",prevMinPh,SIZE_THRESHOLD);
  preferences.getBytes("5",prevMaxPh,SIZE_THRESHOLD);
  preferences.getBytes("6",prevMinTemp,SIZE_THRESHOLD);
  preferences.getBytes("7",prevMaxTemp,SIZE_THRESHOLD);
  preferences.getBytes("8",prevMinTds,SIZE_THRESHOLD);
  preferences.getBytes("9",prevMaxTds,SIZE_THRESHOLD);
  StoreNewFlashData("0",subTopic.getValue(),prevSubTopic,SIZE_TOPIC);
  StoreNewFlashData("A",clientID.getValue(),prevClientID,SIZE_CLIENT_ID);
  StoreNewFlashData("2",channelId.getValue(),prevChannelId,SIZE_CHANNEL_ID);
  StoreNewFlashData("3",apiKey.getValue(),prevApiKey,SIZE_API_KEY); 
  StoreNewFlashData("4",minPh.getValue(),prevMinPh,SIZE_THRESHOLD); 
  StoreNewFlashData("5",maxPh.getValue(),prevMaxPh,SIZE_THRESHOLD); 
  StoreNewFlashData("6",minTemp.getValue(),prevMinTemp,SIZE_THRESHOLD);
  StoreNewFlashData("7",maxTemp.getValue(),prevMaxTemp,SIZE_THRESHOLD);
  StoreNewFlashData("8",minTds.getValue(),prevMinTds,SIZE_THRESHOLD);
  StoreNewFlashData("9",maxTds.getValue(),prevMaxTds,SIZE_THRESHOLD);
}
