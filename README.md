# nimble
## Arduino IoT multi-sensor for the ESP8266. 
Supports a number of popular sensors. Simply wire sensors to the ESP8266 and compile this sketch. Use the Http Rest API (Postman collection provided) to configure and control the sensors and direct sensor data to a number of targets such as Influx for analytics or a home automation controller.

# DO NOT USE
This project is still under construction. It is currently working and provides a HTTP Rest interface to a number of sensors. I expect to reach a release version around the middle of May 2019. Work was halted while the Rest code was refactored out into the [Restfully library](https://github.com/flyingeinstein/Restfully) which is now complete.

# Current Sensor Support
* Temperature - Dallas 1-wire
* Humidity & Temperature - DHT sensor
* Motion - any GPIO based digital motion sensor
* Moisture sensors (capacitive or resistive)
* Atlas Scientific EZO Probes such as pH, ORP, Conductance and Dissolved Oxygen probes (ex. for pool chemistry)

# Display Device Support
Currently supports the popular SSD1306 OLED LCD (popular 0.96" OLED display) with more suport to come. You can change the layout of the display pages using the HTTP interface and a simple graphical drawing scripting language.

# Data & Communications
* Simple Web Server
* REST/Json API   (Postman Collection   https://www.getpostman.com/collections/0c7700f851f84ad10845)
* Send to Influx DB
* (soon) MQTT pub/sub model (good for OpenHab)

# Features
* Uses NTP to get time so no RTC needed
* Can set WiFi network via captive portal in SoftAP mode
* Supports Over-the-Air (OTA) software updates
* Registers hostname and services using mDNS

# Development Plan
These items remain in development. As items are completed they are removed from this list.

## Version 1.0
* DeviceManager on() methods handle Rest URLs and handlers of ArduinoJson type
* Add Configuration API
  * Import/Export config via Json API
  * get/set via direct Rest API
  * works very much like Sensor values
  * loads drivers from configuration (not hard coded anymore)
* Get Influx working again
  * set config via Rest
     * function to create Line encodings
     * function to send the Line data

## Version 1.2
* I2C bus has its own Devices object, and i2c buses are a child of I2C bus
     * i2c buses are no longer on the same DeviceManager as i2c bus
     * API now supports SensorAddresses with multiple levels (i.e 1:96:0 would typically be i2c-bus:ezo:pH)
     * might need to change Device class so slots are virtual, not backed by real SensorReading array. Can refactor, and make subclass implement a real array (maybe template based)
     * i2c bus can probe and create devices
     * ReadingIterator iterates within buses
* Atlas Scientific Probes (can already take readings, but no calibration done)
     * Support calibration
     * Support temperature compensation
     * Support slope
     * Support other status/info calls
* Add MQMTT support
* Device/Driver configuration is loaded/saved via Config API

## Version 1.4 - React App
* Add React web app and store in SPIFFS
* Can set aliases and configuration
* Visual KPIs and Dashboards
   
# IDEAS
* Convert NTP to be a sensor (time sensor). It has a few timestamps as readings. Also, other sensors can have a slot that outputs a timestamp, such as timestamp since last event.
* Perhaps Device class should just have a getReading() virtual method instead of an array. The sub-class can then decide to get the value on-request.
* Enable cloud support (either customer created or mine)
     * brings all a users nimbles together
     * enables sharing of nimbles with other users and groups (like weather data, power, gps) for crowd-sourced analytics
   
   
