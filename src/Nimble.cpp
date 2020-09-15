
// Nimble IoT Multi-Sensor
// Written by Colin MacKenzie, MIT license
// (c)2018 FlyingEinstein.com

// If set, enables a captive portal by creating an access point
// you can connect to and set the actual Wifi network to connect.
// Enabling captive portal can take significant program resources.
//#define CAPTIVE_PORTAL

//#define ALLOW_OTA_UPDATE

// hard code the node name of the device
const char* hostname = SSID_HOSTNAME;

// if you dont use the Captive Portal for config you must define
// the SSID and Password of the network to connect to.
#if !defined(CAPTIVE_PORTAL)
const char* ssid = SSID_NAME;
const char* password = SSID_PASSWORD;
#endif


bool enable_influx = false;
//const char* influx_server = INFLUX_SERVER;
const char* influx_database = INFLUX_DATABASE;
const char* influx_measurement = INFLUX_MEASUREMENT;

#include <FS.h>   // Include the SPIFFS library

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


#include "ModuleSet.h"
#include "ModuleManager.h"
#include "Motion.h"
#include "AnalogPin.h"
#include "DHTSensor.h"
#include "OneWireSensors.h"
#include "Display.h"
#include "AtlasScientific.h"
#include "NtpClient.h"
#include "Influx.h"
#include "GelfLogger.h"

#include <Restfully.h>

// our fonts
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSans12pt7b.h>
#include <Fonts/FreeSans18pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include <Fonts/FreeSansBoldOblique9pt7b.h>
#include <Fonts/FreeMono9pt7b.h>
#include <Fonts/FreeMono12pt7b.h>
#include <Fonts/FreeMono18pt7b.h>
#include <Fonts/Org_01.h>
#include <Fonts/Picopixel.h>
//#include <Fonts/Tiny3x3a2pt7b.h>

const FontInfo display_fonts[] = {  
  FONT(FreeSans9pt7b),
  FONT(FreeSans12pt7b),
  FONT(FreeSans18pt7b),
  FONT(FreeSansBold9pt7b),
  FONT(FreeSansBoldOblique9pt7b),
  FONT(FreeMono9pt7b),
  FONT(FreeMono12pt7b),
  FONT(FreeMono18pt7b),
  FONT(Org_01),
  FONT(Picopixel),
//  FONT(Tiny3x3a2pt7b)
};
Display* display;



ESP8266WebServer server(80);


#if defined(CAPTIVE_PORTAL)
AutoConnect Portal(server);
#endif



/*** Web Server

*/
void SendHeaders()
{
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
  server.sendHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE");
}

String getContentType(String filename) { // convert the file extension to the MIME type
  if (filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".css")) return "text/css";
  else if (filename.endsWith(".js")) return "application/javascript";
  else if (filename.endsWith(".ico")) return "image/x-icon";
  else if (filename.endsWith(".txt")) return "text/plain";
  return "text/plain";
}

bool handleFileRead(String path) { // send the right file to the client (if it exists)
  Serial.println("handleFileRead: " + path);
  //if (path.endsWith("/")) path += "index.html";         // If a folder is requested, send the index file
  String contentType = getContentType(path);            // Get the MIME type
  if (SPIFFS.exists(path)) {                            // If the file exists
    File file = SPIFFS.open(path, "r");                 // Open it
    /*size_t sent = */ server.streamFile(file, contentType); // And send it to the client
    file.close();                                       // Then close the file again
    return true;
  }
  Serial.println("\tFile Not Found");
  return false;                                         // If the file doesn't exist, return false
}

void handleRoot() {
  String html = "<html><head><title>Wireless Wall SensorInfo</title>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<link rel='stylesheet' href='css/nimble.css'>";
  html += "<meta http-equiv=\"refresh\" content=\"30\">";
  html += "</head>";
  html += "<body>";
  // header
  html += "<div class='header'><h1>Nimble Sensor</h1><span class='copyright'>Copyright 2018, Flying Einstein LLC</span></div>";
  // title area 
  html += "<div class='title'><h2><label>Site</label> ";
  html += hostname;
  html += "</h2></div>";

  html += "<div class='tiles'>";

  // enumerate through all sensor types, and then iterate all sensors of that type
  // so that sensors of the same group are grouped together
  Nimble::SensorReading r;
  for(short dt=Nimble::FirstSensorType; dt <= Nimble::LastSensorType; dt++) {
    Nimble::ModuleSet::ReadingIterator itr = Nimble::ModuleManager::Default.modules().forEach((Nimble::SensorType)dt);
    short n = 0;
    String typeName = Nimble::SensorTypeName((Nimble::SensorType)dt);
    
    while( (r = itr.next()) ) {
      if(n==0) {
        // first device, add a header
        html += "<div class='sensors'><h3>";
        html += typeName;
        html += "</h3>";
      }

      html += "<div id='";
      html += typeName;
      html += "' class='sensor'>";

      String alias = itr.device->getSlotAlias(itr.slot);
      if(alias.length()) {
        html += "<label class='alias'>";
        html += alias;
        html += "</label>";
      }
      
      html += "<span>";
      html += r.toString();
      html += "</span>";      
      
      html += "<label class='address'>";
      html += itr.device->id;
      html += ':';
      html += itr.slot;
      html += "</label>";
      
      html += "</div>";
      n++;
    }

    if(n>0)
      html += "</div>"; // closing tag for the group
  }

  html += "</div></body></html>";
  server.send(200, "text/html", html);
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
  ESP.wdtDisable();
  ESP.wdtEnable(WDTO_8S);
  
  Serial.begin(115200);
  Serial.println("Nimble Multi-Sensor");
  Serial.println("(c)2018 FlyingEinstein.com");

  SPIFFS.begin();                           // Start the SPI Flash Files System
 
  server.addHandler(&optionsRequestHandler);
  server.on("/", handleRoot);

  //server.onNotFound(handleNotFound);
  // our 404 handler tries to load documents from SPIFFS
  server.onNotFound([]() {                              // If the client requests any URI
    if (!handleFileRead(server.uri()))                  // send it if it exists
      handleNotFound();                                 // send the 404 message if it doesnt exist
  });
  
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

  /**
   *   BEGIN SENSOR CONFIG
   *   
   *   todo: Eventually this will be configurable or via probing where possible
   * 
   */
  auto& manager = Nimble::ModuleManager::Default;
  Nimble::ModuleSet& modules = manager.modules();
  Nimble::ModuleManager::Default.begin( server, ntp );
  modules.add( *new OneWireSensor(5, 2) );   // D4
  modules.add( *(display = new Display()) );             // OLED on I2C bus
  modules.add( *new GelfLogger(3) );             // Graylog logger
  modules.add( *new DHTSensor(4, 14, DHT22) );      // D5
  modules.add( *new MotionIR(6, 12) );       // D6

  // we can optionally add the I2C bus as a device which enables external control
  // but without this i2c devices will default to using the system i2c bus
  //ModuleManager.add( *new I2CBus(2) );       // Place Wire bus at 1:0
  AtlasScientific::EzoProbe* pHsensor = new AtlasScientific::EzoProbe(8, Nimble::pH);
  //pHsensor->setBus( SensorAddress(2,0) );   // attach i2c sensor to a specific bus
  modules.add( *pHsensor );       // pH probe at 8 using default i2c bus
  
  display->setFontTable(display_fonts);

  manager.restoreAliasesFile();

  String hostinfo;
  hostinfo += hostname;
  hostinfo += " (IP: ";
  hostinfo += WiFi.localIP().toString();
  hostinfo += ")";
  Serial.print("Host: ");
  Serial.println(hostinfo);

  manager.logger().warn("startup")
    .module("core")
    .detail("device "+hostinfo+" initialized");
}


unsigned long nextInfluxWrite = 0;

void loop() {

#if defined(ALLOW_OTA_UPDATE)
  ArduinoOTA.handle();
#endif
  
#if defined(CAPTIVE_PORTAL)
  Portal.handleClient();
#else
  server.handleClient();
#endif

  // no further processing if we are not in station mode
  if(WiFi.getMode() != WIFI_STA)
    return;

  Nimble::ModuleManager::Default.handleUpdate();

#ifdef ENABLE_INFLUX
  if (enable_influx && influx_server != NULL && millis() > nextInfluxWrite) {
    nextInfluxWrite = millis() + 60000;
    sendToInflux();
  }
#endif
}
