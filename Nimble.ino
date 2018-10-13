
// Nimble IoT Multi-Sensor
// Written by Colin MacKenzie, MIT license
// (c)2018 FlyingEinstein.com

// If set, enables a captive portal by creating an access point
// you can connect to and set the actual Wifi network to connect.
// Enabling captive portal can take significant program resources.
#define CAPTIVE_PORTAL

//#define ALLOW_OTA_UPDATE

// hard code the node name of the device
const char* hostname = "hallway";

// if you dont use the Captive Portal for config you must define
// the SSID and Password of the network to connect to.
#if !defined(CAPTIVE_PORTAL)
const char* ssid = "";
const char* password = "";
#endif

boolean enable_influx = false;
const char* influx_server = "http://192.168.2.115";
const char* influx_database = "gem";
const char* influx_measurement = "walls";

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <WiFiUdp.h>
#include <NTPClient.h>

#if defined(ALLOW_OTA_UPDATE)
#include <ArduinoOTA.h>
#endif

// AutoConnect library by Hieromon
// provides Captive AccessPoint for initial node configuration
#if defined(CAPTIVE_PORTAL)
#include <AutoConnectPage.h>
#include <AutoConnect.h>
#include <AutoConnectCredential.h>
#endif

#include <DHT.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#include "Sensors.h"

#define DHTPIN 12     // what digital pin we're connected to
#define ONE_WIRE_BUS  4

// Dallas 1wire temperature sensor resolution
#define SENSOR_RESOLUTION 9

#define ENABLE_DHT
//#define ENABLE_MOISTURE
//#define ENABLE_MOTION
//#define ENABLE_INFLUX
#define LCD SSD1306

// Libraries OLED
#if (LCD == SSD1306)
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#define OLED_RESET LED_BUILTIN        // as per https://maker.pro/arduino/projects/oled-i2c-display-arduinonodemcu-tutorial
Adafruit_SSD1306 display(OLED_RESET);
#endif

// Uncomment whatever type you're using!
//#define DHTTYPE DHT11   // DHT 11
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
//#define DHTTYPE DHT21   // DHT 21 (AM2301)

#define TIMESTAMP_MIN  1500000000   // time must be greater than this to be considered NTP valid time

// Connect pin 1 (on the left) of the sensor to +5V
// NOTE: If using a board with 3.3V logic like an Arduino Due connect pin 1
// to 3.3V instead of 5V!
// Connect pin 2 of the sensor to whatever your DHTPIN is
// Connect pin 4 (on the right) of the sensor to GROUND
// Connect a 10K resistor from pin 2 (data) to pin 1 (power) of the sensor

// Initialize DHT sensor.
// Note that older versions of this library took an optional third parameter to
// tweak the timings for faster processors.  This parameter is no longer needed
// as the current DHT reading algorithm adjusts itself to work on faster procs.
DHT dht(DHTPIN, DHTTYPE);

// Dallas Semi 1-wire temp sensors
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature DS18B20(&oneWire);

short sensor_read_motion(const SensorInfo& sensor, Outputs& outputs);

typedef enum {
  JsonName,
  JsonValue,
  JsonFull
} JsonPart;

extern InfluxTarget targets[] = {
  { "gem", "walls" },
  { "gem", "motion" }
};

SensorInfo sensors[] = {
  {
    /* type             */ Motion,
    /* name             */ "hall",
    /* target           */ targets[1],
    /* pin              */ 14,
    /* update frequency */ 1000,
    /* read function    */ sensor_read_motion
  }
};

ESP8266WebServer server(80);
AutoConnect Portal(server);

WiFiUDP ntpUDP;
NTPClient ntp(ntpUDP);


class Data {
  public:
    bool humidityPresent;
    float humidity;
    float heatIndex;

    bool moisturePresent;
    int moisture;

    int temperatureCount;
    float temperature[32];

    unsigned long timestamp;
};

Data data;
Outputs readings;


/*** Web Server

*/
void SendHeaders()
{
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
  server.sendHeader("Access-Control-Allow-Methods", "GET");
}

void handleRoot() {
  String status = "<html><head><title>Wireless Wall SensorInfo</title><meta http-equiv=\"refresh\" content=\"30\"></head>";
  status += "<style>table tbody th { text-align: right; padding-right: 1em }  h4 label { font-size: 80%; }</style>";
  status += "<body>";
  status += "<h2>Wireless Wall SensorInfo</h2>";
  status += "<h3><label>Site</label> ";
  status += hostname;
  status += "</h3>";
  status += "<table><tbody>";

  status += "<tr><th>Timestamp</th><td>";
  status += data.timestamp;
  status += "</td></tr>";

  status += "<tr><th>Humidity</th><td>";
  status += data.humidity;
  status += "</td></tr>";

  status += "<tr><th>Heat Index</th><td>";
  status += data.heatIndex;
  status += "</td></tr>";

  status += "<tr><th>Moisture</th><td>";
  status += data.moisture;
  status += "</td></tr>";

  for (int i = 0; i < data.temperatureCount; i++) {
    status += "<tr><th>T";
    status += i;
    status += "</th><td>";
    status += data.temperature[i];
    status += "</td></tr>";
  }
  status += "</tbody></table>";
  status += "<h6>Copyright 2018, Flying Einstein LLC</h6></body></html>";
  server.send(200, "text/html", status);
}

void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

class OptionsRequestHandler : public RequestHandler
{
    virtual bool canHandle(HTTPMethod method, String uri) {
      return method == HTTP_OPTIONS;
    }
    virtual bool handle(ESP8266WebServer& server, HTTPMethod requestMethod, String requestUri) {
      SendHeaders();
      server.send(200, "application/json; charset=utf-8", "");
      return true;
    }
} optionsRequestHandler;


#ifdef ENABLE_INFLUX
void sendToInflux()
{
  if (WiFi.status() != WL_CONNECTED)
    return;
  if (data.timestamp < TIMESTAMP_MIN) // some time in 2017, before this code was built
    return;
  unsigned long stopwatch = millis();
  String url = influx_server;
  url += ":8086/write?precision=s&db=";
  url += influx_database;

  int fields = 0;
  String post = influx_measurement;
  post += ",site=";
  post += hostname;
  post += " ";

  if (data.humidityPresent) {
    post += "humidity=";
    post += data.humidity;
    post += ",heatIndex=";
    post += data.heatIndex;
    fields += 2;
  }
  if (data.moisturePresent) {
    if (fields > 0)
      post += ",";
    post += "moisture=";
    post += data.moisture;
  }
  for (int i = 0; i < data.temperatureCount; i++) {
    if (fields > 0)
      post += ",";
    post += "t";
    post += i;
    post += "=";
    post += data.temperature[i];
  }

  // timestamp
  post += " ";
  post += data.timestamp;

  if (true) {
    HTTPClient http;
    http.begin(url);
    http.addHeader("Content-Type", "text/plain");
    int httpCode = http.POST(post);
    //String payload = http.getString();
    //Serial.print("influx: ");
    //Serial.println(payload);
    http.end();

    stopwatch = millis() - stopwatch;
    Serial.print("influx post in ");
    Serial.print(stopwatch);
    Serial.print(" millis");
  }
  //Serial.print(url);
  Serial.print(" => ");
  Serial.println(post);
}
#endif

void setup() {
  // Initiate the LCD and disply the Splash Screen
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C, false);  // initialize with the I2C addr 0x3C (for the 128x32)
  display.display();
  display.clearDisplay();

  Serial.begin(230400);
  Serial.println("Nimble Multi-Sensor");
  Serial.println("(c)2018 FlyingEinstein.com");

  server.addHandler(&optionsRequestHandler);
  server.on("/", handleRoot);
  server.on("/status", JsonSendStatus);
  server.on("/devices", JsonSendDevices);

  server.onNotFound(handleNotFound);
  
#if defined(CAPTIVE_PORTAL)
  AutoConnectConfig  Config("nimble", "chillidog");    // SoftAp name is determined at runtime
  /*Config.apid = "nimble";//ESP.hostname();                 // Retrieve host name to SoftAp identification
  Config.apip = IPAddress(192,168,10,101);      // Sets SoftAP IP address
  Config.gateway = IPAddress(192,168,10,1);     // Sets WLAN router IP address
  Config.netmask = IPAddress(255,255,255,0);    // Sets WLAN scope
  Config.autoReconnect = true;                  // Enable auto-reconnect
  Config.autoSave = AC_SAVECREDENTIAL_NEVER;    // No save credential
  Config.boundaryOffset = 64;                   // Reserve 64 bytes for the user data in EEPROM. 
  Config.homeUri = "/index.html";               // Sets home path of the sketch application
  Config.staip = IPAddress(192,168,10,10);      // Sets static IP
  Config.staGateway = IPAddress(192,168,10,1);  // Sets WiFi router address
  Config.staNetmask = IPAddress(255,255,255,0); // Sets WLAN scope
  Config.dns1 = IPAddress(192,168,10,1);        // Sets primary DNS address*/
  Portal.config(Config);                        // Configure AutoConnect

  Portal.begin();
#else
  WiFi.mode(WIFI_STA);
  WiFi.hostname(hostname);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
#ifdef SERIAL_DEBUG
    Serial.println("Connection Failed! Rebooting...");
#endif
    delay(5000);
    ESP.restart();
  }

  server.begin();
#endif

#if defined(ALLOW_OTA_UPDATE)
  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);
  ArduinoOTA.setHostname(hostname);

  // No authentication by default
  //ArduinoOTA.setPassword("");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else // U_SPIFFS
      type = "filesystem";

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
#ifdef SERIAL_DEBUG
    Serial.println("Start updating " + type);
#endif
  });
  ArduinoOTA.onEnd([]() {
#ifdef SERIAL_DEBUG
    Serial.println("\nEnd");
#endif
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
#ifdef SERIAL_DEBUG
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
#endif
  });
  ArduinoOTA.onError([](ota_error_t error) {
#ifdef SERIAL_DEBUG
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
#endif
  });
  ArduinoOTA.begin();
#endif

  // add the web server to mDNS
  MDNS.begin(hostname);
  MDNS.addService("http", "tcp", 80);

  // begin the ntp client
  ntp.begin();

  // humidity sensor
  dht.begin();

  // setup OneWire bus
  DS18B20.begin();
  //DS18B20.setResolution(sensorDeviceAddress, SENSOR_RESOLUTION);

  pinMode(14, INPUT);
  pinMode(DHTPIN, INPUT);

  Serial.print("Host: ");
  Serial.print(hostname);
  Serial.print("   IP: ");
  Serial.println(WiFi.localIP());
}

#define LCD_COL1  45
#define LCD_COL2  85
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSans12pt7b.h>
#include <Fonts/FreeSansBoldOblique9pt7b.h>
#include <Fonts/Org_01.h>

#define TITLE_FONT FreeSans12pt7b
#define  DATA_FONT FreeSans9pt7b
#define LINE_HEIGHT 12

unsigned long long lastMotion = 0;

void OLED_Display()
{
  int y = 16;
  unsigned long alone = millis() - lastMotion;

  if(alone > 30000) {
    display.clearDisplay();
    display.display();
    return;
  }
  display.dim( alone > 15000 );

  //display.ssd1306_command(SSD1306_SETCONTRAST);
  //display.ssd1306_command(255); // Whereama c is a value from 0 to 255 (sets contrast e.g. brightness)
  
  // Update OLED values
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setFont(&TITLE_FONT);
  display.setTextSize(1);
  display.setCursor(0, y);
  display.print(data.temperature[0], 1);

  //display.setTextSize(1);
  display.setFont(NULL);
  display.drawCircle(display.getCursorX()+3, 8, 1, WHITE);
  display.setCursor(display.getCursorX()+1, 8);
  display.print(" F");

  display.setFont(&FreeSans9pt7b);
  //display.setTextSize(1);
  y += FreeSans9pt7b.yAdvance; //LINE_HEIGHT;
  display.setCursor(0, y);
  display.print(hostname);

  // display motion devices
  for (int i = 0, x=120; i < readings.count; i++) {
    SensorReading r = readings.values[i];
    if(r.sensorType!=Motion)
      continue;
    if(r.b)
      display.fillCircle(x, y-4, 4, WHITE);
    else
      display.drawCircle(x, y-4, 4, WHITE);
    x -= 8;
  }
  
  display.setFont(NULL);

#if 0
  //display.setTextSize(1);
  y = 64 - LINE_HEIGHT;
  display.setCursor(0, y);
  display.print("HI");
  display.setCursor(LCD_COL1, y);
  display.print(data.heatIndex, 1);
  display.print("\xB0C");
  
  y -= LINE_HEIGHT;
  display.setCursor(0, y);
  display.print("RH");
  display.setCursor(LCD_COL1, y);
  display.print(data.humidity, 1);
  display.print("%");
#else
  y = 0;
  display.setCursor(LCD_COL2, y);
  display.print("HI");
  display.setCursor(LCD_COL2+15, y);
  display.print(data.heatIndex, 1);
  
  y += LINE_HEIGHT;
  display.setCursor(LCD_COL2, y);
  display.print("RH");
  display.setCursor(LCD_COL2+15, y);
  display.print(data.humidity, 1);
  //display.print("%");  
#endif

  display.display();
}

void PrintData(Data& data)
{
  // print data to console
  Serial.print("T");
  Serial.print(data.timestamp);
  if (data.humidityPresent) {
    Serial.print("  RH ");
    Serial.print(data.humidity);
    Serial.print("%\t");
    Serial.print("HI ");
    Serial.print(data.heatIndex);
    Serial.print("*F  ");
  }
  if (data.moisturePresent) {
    Serial.print("Moisture ");
    Serial.print(data.moisture);
    Serial.print("  ");
  }
  if (data.temperatureCount > 0) {
    Serial.print("T[");
    for (int i = 0; i < data.temperatureCount; i++) {
      if (i > 0)
        Serial.print(" ");
      Serial.print(data.temperature[1]);
    }
    Serial.print("]");
  }
  Serial.println();
}

short sensor_read_motion(const SensorInfo& sensor, Outputs& readings)
{
  // read a typical motion sensor on a digital pin
  bool motion = digitalRead(sensor.pin) ? true : false;
  if(motion)
    lastMotion = millis();
  readings.write( SensorReading(sensor, motion) );
}

unsigned long long nextRead = 0;
unsigned long long nextDisplayUpdate = 0;
uint8_t state = 0;
uint8_t attempts = 0;
Data newdata;

enum {
  STATE_PREAMBLE,
  STATE_READ_HUMIDITY,
  STATE_READ_MOISTURE,
  STATE_READ_1WIRE,
  STATE_READ_MOTION,
  STATE_SAVE_DATA,
  STATE_POST_INFLUX
};

void loop() {
  float h, f;
  uint8_t count;

#if defined(ALLOW_OTA_UPDATE)
  ArduinoOTA.handle();
#endif
  
  Portal.handleClient();

  // no further processing if we are not in station mode
  if((WiFi.getMode() != WIFI_STA) ==0)
    return;
    
  ntp.update();

#if defined(LCD)
  if (millis() > nextDisplayUpdate) {
    OLED_Display();
    nextDisplayUpdate = millis() + 400;
  }
#endif

#if 0
  // allow sensors to update as updateFrequency
  readings.reset();
  for (int i = 0; i < sizeof(sensors) / sizeof(sensors[0]); i++) {
    const SensorInfo& sensor = sensors[i];
    short rv = sensor.read(sensor, readings);
  }
#endif

  switch (state) {
    case STATE_PREAMBLE:
      if (millis() > nextRead) {
        memset(&newdata, 0, sizeof(newdata));
        newdata.timestamp = ntp.getEpochTime();
        if (newdata.timestamp > TIMESTAMP_MIN) {
          state++;
          attempts = 0;
        } else {
          // wait another x millis for NTP to complete
          Serial.println("ntp not ready");
          nextRead = millis() + 1000;  // now we defer the next read attempt for X seconds
        }
      }
      break;
    case STATE_READ_HUMIDITY:
#ifdef ENABLE_DHT
      if (millis() > nextRead) {
        // Reading temperature or humidity takes about 250 milliseconds!
        // SensorInfo readings may also be up to 2 seconds 'old' (its a very slow sensor)
        h = dht.readHumidity();
        // Read temperature as Fahrenheit (isFahrenheit = true)
        f = dht.readTemperature(true);
  
        // Check if any reads failed and exit early (to try again).
        if (isnan(h) || isnan(f)) {
          Serial.println("Failed to read from DHT sensor!");
          if (++attempts > 3) 
            state++;  // skip humidity
          else {
            nextRead = millis() + 1000;  // now we defer the next read attempt for X seconds
            delay(100);
          }
          return;  
        }
  
        // update data
        newdata.humidityPresent = true;
        newdata.humidity = h;
        newdata.heatIndex = dht.computeHeatIndex(f, h);
        newdata.temperature[newdata.temperatureCount++] = f;
      }
#endif
      state++;
      break;
    case STATE_READ_MOISTURE:
#ifdef ENABLE_MOISTURE
      newdata.moisture = 1024 - analogRead(A0);
      newdata.moisturePresent = true;
#endif
      state++;
      break;
    case STATE_READ_1WIRE:
      // read Dallas DS18 temperature devices
      count = DS18B20.getDS18Count();
      DS18B20.requestTemperatures();
      for (int i = 0; i < count; i++) {
        newdata.temperature[newdata.temperatureCount++] = DS18B20.getTempFByIndex(i);
      }
      state++;
      break;
    case STATE_READ_MOTION:
      state++;
      break;
    case STATE_SAVE_DATA:
      if (newdata.humidityPresent || newdata.moisturePresent || newdata.temperatureCount > 0) {
        data = newdata;
        PrintData(data);
        state++;
      } else
        state = 0;
      break;

    case STATE_POST_INFLUX:
#ifdef ENABLE_INFLUX
      if (enable_influx && influx_server != NULL)
        sendToInflux();
#endif
      state++;
      break;

    default:
      nextRead = millis() + 5000;  // now we defer the next read attempt for X seconds
      state = 0;  // back to beginning
      break;
  }

}


void JsonAddSensorReadings(String& json, SensorType st, JsonPart part)
{
  json += ",  \"";
  json += SensorTypeName(st);
  json += "\": [";
  for (int i = 0; i < readings.count; i++) {
    SensorReading r = readings.values[i];
    if(r.sensorType!=st)
      continue;
    if (i > 0) json += ", ";
    if (part == JsonValue) {
      json += r.toString();
    } else if (part == JsonName) {
      json += r.sensor ? r.sensor->name : "n/a";
    } else if (part == JsonFull) {
      json += "{ \"type\": \"";
      json += SensorTypeName(r.sensorType);
      json += "\", \"ts\": ";
      json += r.timestamp;
      json += ", \"value\": ";
      json += r.toString();
      json += "}";
    }
  }
  json += "]\n";
}

void JsonSendStatus() {
  String status = "{\n  \"site\": \"";
  status += hostname;
  status += "\"";
  if (data.timestamp > TIMESTAMP_MIN) {
    status += ",  \"timestamp\": ";
    status += data.timestamp;
  }
  if (data.humidityPresent) {
    status += ",  \"humidity\": ";
    status += data.humidity;
    status += ",  \"heatIndex\": ";
    status += data.heatIndex;
  }
  if (data.moisturePresent) {
    status += ",  \"moisture\": ";
    status += data.moisture;
  }

  status += ",  \"temperatures\": [";
  for (int i = 0; i < data.temperatureCount; i++) {
    if (i > 0)
      status += ",";
    status += data.temperature[i];
  }
  status += "]\n";

  JsonAddSensorReadings(status, Motion, JsonValue);

  status += "}";
  SendHeaders();
  server.send(200, "text/html", status);
}

const char* hex = "0123456789ABCDEF";

void JsonSendDevices() {
  String status = "{\n  \"site\": \"";
  status += hostname;
  status += "\",  \"humidity\": ";
  status += data.humidityPresent ? "true" : "false";
  status += ",  \"moisture\": \"";
  status += data.moisturePresent ? "true" : "false";
  status += "\",";

  status += "  \"temperatureDevices\": [";
  uint8_t devices = 0;
  if (data.humidityPresent) {
    devices++;
    status += "\"DHT\"";
  }
  uint8_t count = DS18B20.getDS18Count();
  DeviceAddress devaddr;
  for (int i = 0; i < count; i++) {
    if (devices > 0)
      status += ",";
    if (DS18B20.getAddress(devaddr, i)) {
      status += "\"";
      for (int j = 0; j < 8; j++) {
        if (j > 0)
          status += ":";
        status += hex[ devaddr[j] / 16 ];
        status += hex[ devaddr[j] % 16 ];
      }
      status += "\"";
    }
    devices++;
  }
  status += "]\n";

  JsonAddSensorReadings(status, Motion, JsonName);

  status += "}\n";
  SendHeaders();
  server.send(200, "text/html", status);
}
