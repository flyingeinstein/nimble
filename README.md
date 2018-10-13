# nimble
Arduino IoT multi-sensor for the ESP8266

# DO NOT USE
This code is under significant rewriting, come back around Halloween 2018 to check out again.

# Current Sensor Support
* Temperature - Dallas 1-wire
* Humidity & Temperature - DHT sensor
* Motion - any GPIO based digital motion sensor
* Moisture sensors (capacitive or resistive)

# Other Device Support
* SSD1306 OLED LCD (popular 0.96" OLED display)

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

