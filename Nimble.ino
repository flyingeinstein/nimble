
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

#define ENABLE_DHT
//#define ENABLE_MOISTURE
//#define ENABLE_MOTION
//#define ENABLE_INFLUX


#include "DeviceManager.h"
#include "Motion.h"
#include "AnalogPin.h"
#include "DHTSensor.h"
#include "OneWireSensors.h"
#include "Display.h"

// our fonts
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSans12pt7b.h>
#include <Fonts/FreeSansBoldOblique9pt7b.h>
#include <Fonts/Org_01.h>

const GFXfont* display_fonts[] = {  
  NULL,           // will be the built-in font
  &FreeSans12pt7b,
  &FreeSans9pt7b,
  &FreeSansBoldOblique9pt7b,
  &Org_01
};
Display display;
char* pages[6] = { NULL };

#define TIMESTAMP_MIN  1500000000   // time must be greater than this to be considered NTP valid time

typedef enum {
  JsonName,
  JsonValue,
  JsonFull
} JsonPart;

extern InfluxTarget targets[] = {
  { "gem", "walls" },
  { "gem", "motion" }
};

#if 0
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
#endif

ESP8266WebServer server(80);

#if defined(CAPTIVE_PORTAL)
AutoConnect Portal(server);
#endif

WiFiUDP ntpUDP;
NTPClient ntp(ntpUDP);

/*
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
*/

void setPageCode(short page, const char* code)
{
  if(page < (sizeof(pages)/sizeof(pages[0]))) {
    if(pages[page])
      free(pages[page]);
    pages[page] = strdup(code); //malloc(strlen(code)+1);
  }
}

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
#if 0
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
  #endif
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

#if 0
void JsonAddSensorReadings(String& json, SensorType st, JsonPart part)
{
  Devices::ReadingIterator itr = DeviceManager.forEach(st);
  json += ",  \"";
  json += SensorTypeName(st);
  json += "\": [";
  SensorReading r;
  while ( r = itr.next() ) {
    if (i > 0) json += ", ";
    if (part == JsonValue) {
      json += r.toString();
    //} else if (part == JsonName) {
    //  json += itr.device ? itr.device->name : "n/a";
    } else if (part == JsonFull) {
      json += "{ \"type\": \"";
      json += SensorTypeName(r.sensorType);
      json += "\", \"device\": ";
      json += itr.device->id;
      json += "\", \"slot\": ";
      json += itr.slotOrdinal;
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
#endif

void setup() {
  Serial.begin(230400);
  Serial.println("Nimble Multi-Sensor");
  Serial.println("(c)2018 FlyingEinstein.com");

  DeviceManager.begin( ntp );
  DeviceManager.add( new DHTSensor(2, 12) );      // D6
  //DeviceManager.add( new OneWireSensor(1, 2) );   // D4
  DeviceManager.add( new MotionIR(6, 14) );       // D5
  
  display.begin(DeviceManager, display_fonts);
  setPageCode(0, "G1R0C0'RH \nG2D2S0\nG1R1C0'T  \nG2S1");
  display.execute(pages[0]);

  server.addHandler(&optionsRequestHandler);
  server.on("/", handleRoot);
  //server.on("/status", JsonSendStatus);
  //server.on("/devices", JsonSendDevices);

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

  Serial.print("Host: ");
  Serial.print(hostname);
  Serial.print("   IP: ");
  Serial.println(WiFi.localIP());
}


void PrintData(SensorType st)
{
  int n=0;
  SensorReading r;
  Devices::ReadingIterator itr = DeviceManager.forEach(st);
  while( r = itr.next() ) {
    if(n==0) {
      Serial.print(SensorTypeName(st));
      Serial.print(": ");
    } else {
      Serial.print(", ");
    }
    Serial.print(itr.device->id);
    Serial.print(':');
    Serial.print(itr.slot);
    Serial.print('=');
    Serial.print(r.toString());
    
    n++;
  }
  if(n>0) Serial.println();
}

unsigned long long nextRead = 0;
unsigned long long nextDisplayUpdate = 0;
unsigned long long nextPrintUpdate = 0;
unsigned long nextInfluxWrite = 0;
uint8_t state = 0;

enum {
  STATE_PREAMBLE,
  STATE_SAVE_DATA,
  STATE_POST_INFLUX
};

void loop() {

#if defined(ALLOW_OTA_UPDATE)
  ArduinoOTA.handle();
#endif
  
#if defined(CAPTIVE_PORTAL)
  Portal.handleClient();
#else
  server.handleClient();
#endif

#if defined(LCD)
  if (millis() > nextDisplayUpdate) {
    if(pages[0])
      display.execute(pages[0]);
    nextDisplayUpdate = millis() + 400;
  }
#endif

  // no further processing if we are not in station mode
  if(WiFi.getMode() != WIFI_STA)
    return;

  ntp.update();

  DeviceManager.handleUpdate();

  if (millis() > nextPrintUpdate) {
    Serial.println();
    PrintData(Motion);
    PrintData(Humidity);
    PrintData(Temperature);
    nextPrintUpdate = millis() + 1000;
  }

#ifdef ENABLE_INFLUX
  if (enable_influx && influx_server != NULL && millis() > nextInfluxWrite) {
    nextInfluxWrite = millis() + 60000;
    sendToInflux();
  }
#endif
}
