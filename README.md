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

# Display Module Support
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

# Dependancies
* ArduinoJson
* Restfully by me (Colin MacKenzie)
* Adafruit GFX Library by Adafruit 
* Adafruit SSD1306 by Adafruit
* Adafruit DHT sensor library
* Adafruit Unified Sensor
* NTPClient by Fabrice Wienberg
* OneWire from Paul Stoffregen
* Dallas Temperature by Miles Burton, Tim Newsome, Guil Barros, Rob Tillaart

# Optional Dependancies
* AutoConnect library by Hieromon (provides Captive AccessPoint for initial node configuration)

# Development Progress
You can see my log of development in [TO DO](TODO.md)

