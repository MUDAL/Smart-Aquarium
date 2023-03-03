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
6. Periodic uploading of sensor data to a mobile phone or computer via MQTT for remote access and analysis.    
7. Wi-Fi provisioning.  

## Challenges and Solutions  
During the development of this system, I encountered several challenges, such as synchronization problems   
between the ESP32 and Arduino Nano and interference issues between the PH and TDS sensors. To overcome these issues,   
I ensured that the Arduino Nano was fully initialized before the ESP32 requested data from it.   
I used transistors to switch off the TDS sensor when readings were taken from the PH sensor.  

## Software  
- Both the ESP32 and Arduino Nano were programmed using C++.   
- The ESP-IDF FreeRTOS was used for concurrent task execution by the ESP32.     
- The HiveMQ broker was used to link the MQTT client running on the mobile phone with the ESP32.    

## Electrical characteristics  
- Running current: approximately 300mA  
- Peak current: 480mA (occurs when all sensors are active and the system isn't connected to a Wi-Fi network)  

![20230214_100451](https://user-images.githubusercontent.com/46250887/218694710-b80014f1-94da-4017-b66a-1ba3daf20b35.jpg)
![20230214_100459](https://user-images.githubusercontent.com/46250887/218694781-e6b665ba-9ee9-4f62-9a08-3a1ccbf7d70a.jpg)
![20230214_100649](https://user-images.githubusercontent.com/46250887/218694830-41035e25-18c0-4081-9bff-4c0f38eed98d.jpg)

## Credits  
1. TDS sensor sample codes: https://wiki.dfrobot.com/Gravity__Analog_TDS_Sensor___Meter_For_Arduino_SKU__SEN0244   
2. Turbidity sensor sample codes and equation: https://wiki.dfrobot.com/Turbidity_sensor_SKU__SEN0189#target_5   
3. Helpful info from ``Electronic Clinic``: https://www.youtube.com/watch?v=rguFeznEELs&t=944s  


