# Smart-Aquarium  

## Description  
The Smart Aquarium System is a device that monitors water quality in real-time for aquariums.  
This system utilizes an ESP32 [Master] and an Arduino Nano [Node] to enable wireless communication and sensor  
data acquisition and management, respectively. It includes multiple sensors, such as PH, TDS, temperature,  
and turbidity sensors, to provide comprehensive information about water quality.   

## Features  
1. Real-time monitoring of water quality for aquariums.   
2. Wireless communication via an ESP32.   
3. Sensor data acquisition and management using an Arduino Nano.   
4. Multiple sensors for comprehensive information about water quality (PH, TDS, temperature, and turbidity sensors).   
5. Periodic display of measured data on an LCD display.   
6. Periodic uploading of sensor data to the ``ThingSpeak`` server for remote access and analysis.
7. Sending of notifications to the user's mobile device via MQTT if any of the parameters are not within a safe range.  
8. Wi-Fi provisioning.  

## Challenges and Solutions  
During the development of this system, I encountered several challenges, such as synchronization problems   
between the ESP32 and Arduino Nano and interference issues between the PH and TDS sensors. To overcome these issues,   
I ensured that the Arduino Nano was fully initialized before the ESP32 requested data from it.   
I used transistors to switch off the TDS sensor when readings were taken from the PH sensor.  

## Software  
- Both the ESP32 and Arduino Nano were programmed using C++.   
- The ESP-IDF FreeRTOS was used for concurrent task execution by the ESP32.     
- The HiveMQ broker was used to link the MQTT client running on the mobile phone with the ESP32.    

## Software architecture  
![ss_sl drawio](https://user-images.githubusercontent.com/46250887/224770505-3b998808-d45e-4bd0-b01b-96e269d378f5.png)  

## Mobile Application  
Notifications from the system are sent to an MQTT broker. The ``HiveMQ`` broker was used in this  
project. The user can use a mobile application (MQTT client) that is linked to the broker to receive notifications  
whenever sensor readings fall out of safe ranges. The safe range for each sensor reading is programmed through the ``captive portal`` during Wi-Fi provisioning.  
The ``MQTT Dashboard`` app from playstore was used. The ``MQTT Alert`` app was also used for testing. Details of the broker used are given below:  

- Broker name: HiveMQ  
- Address: tcp://broker.hivemq.com  
- Port: 1883  

The application (MQTT client) is configured as follows:  
- Subscription topic: Same as the topic configured during Wi-Fi provisioning
- Text box: To display warnings. It subscribes to the ``Subscription topic``.    

Link to download ``MQTT Alert`` application: https://play.google.com/store/apps/details?id=gigiosoft.MQTTAlert  

## Electrical characteristics  
- Running current: approximately 300mA  
- Peak current: 480mA (occurs when all sensors are active and the system is expecting a Wi-Fi network to connect to)  

## Images of prototype  
![IMG-20230519-WA0008](https://github.com/MUDAL/Smart-Aquarium/assets/46250887/6159f407-6ec6-415a-9747-6d91f9b1db5d)  
![20230408_182823](https://user-images.githubusercontent.com/46250887/230764576-9cfaad29-961f-44e2-9f5a-f2a2a43e2f67.jpg)   
![20230408_182829](https://user-images.githubusercontent.com/46250887/230764655-164b67b2-d48e-4b76-bdec-0b83c2cf6dfc.jpg)  
![Meh](https://user-images.githubusercontent.com/46250887/222832138-21d3c1ae-b202-4d82-ab10-050cc7b679d3.jpg)  
![Meh2](https://user-images.githubusercontent.com/46250887/222832470-6972eda9-8dfd-49b4-ab25-4dfadd23d7ae.jpg)  
![20230214_100459](https://user-images.githubusercontent.com/46250887/218694781-e6b665ba-9ee9-4f62-9a08-3a1ccbf7d70a.jpg)
![20230214_100649](https://user-images.githubusercontent.com/46250887/218694830-41035e25-18c0-4081-9bff-4c0f38eed98d.jpg)  
![20230219_135955](https://user-images.githubusercontent.com/46250887/222833043-f97b9952-690e-4195-927c-10c8e25122f5.jpg)  

## Credits  
1. TDS sensor sample codes: https://wiki.dfrobot.com/Gravity__Analog_TDS_Sensor___Meter_For_Arduino_SKU__SEN0244   
2. Turbidity sensor sample codes and equation: https://wiki.dfrobot.com/Turbidity_sensor_SKU__SEN0189#target_5   
3. Helpful info from ``Electronic Clinic``: https://www.youtube.com/watch?v=rguFeznEELs&t=944s  

## Recommendations  
1. Use of accurate sensors for future designs  
2. Power optimization through the selection of better components as well as a more efficient firmware.  

