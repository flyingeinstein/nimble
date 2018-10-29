# nimble
## Arduino IoT multi-sensor for the ESP8266. 
Supports a number of popular sensors. Simply wire sensors to the ESP8266 and compile this sketch. Use the Http Rest API (Postman collection provided) to configure and control the sensors and direct sensor data to a number of targets such as Influx for analytics or a home automation controller.

# DO NOT USE
This project is still under construction. It is currently working and provides a HTTP Rest interface to a number of sensors. I expect to reach a release version around December of 2018.

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
* REST/Json API
* Send to Influx DB
* (soon) MQTT pub/sub model (good for OpenHab)

# Features
* Uses NTP to get time so no RTC needed
* Can set WiFi network via captive portal in SoftAP mode
* Supports Over-the-Air (OTA) software updates
* Registers hostname and services using mDNS

# TODO (to reach version 1.0)
* Support POST for setting Device values
* Convert Display to a Device (since it is one on the i2c bus)
     * Base device class implements the gcode parser, or maybe checkout the Adafruit gfx lib to see how it works with multiple displays and ensure we go on top
* Get Influx working again
  * set config via Rest
     * function to create Line encodings
     * function to send the Line data
* Atlas Scientific Probes
     * Support calibration
     * Support temperature compensation
     * Support slope
     * Support other status/info calls
* Add MQMTT support
   
# IDEAS
* Convert NTP to be a sensor (time sensor). It has a few timestamps as readings. Also, other sensors can have a slot that outputs a timestamp, such as timestamp since last event.
* Perhaps Device class should just have a getReading() virtual method instead of an array. The sub-class can then decide to get the value on-request.
* Enable cloud support (either customer created or mine)
     * brings all a users nimbles together
     * enables sharing of nimbles with other users and groups (like weather data, power, gps) for crowd-sourced analytics
   
   
