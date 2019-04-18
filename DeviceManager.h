
#pragma once


#if (ARDUINO >= 100)
 #include <Arduino.h>
#else
 #include <WProgram.h>
 #include <pins_arduino.h>
#endif
#include <WString.h>
#include <assert.h>

#include <NTPClient.h>

#if defined(ARDUINO_ARCH_ESP8266)
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>

using WebServer = ESP8266WebServer;

#elif defined(ARDUINO_ARCH_ESP32)
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WebServer.h>
#include <HTTPClient.h>
#endif

#include <WiFiUdp.h>
#include <NTPClient.h>

#include <Restfully.h>

#define MAX_SLOTS     256


typedef struct _InfluxTarget {
  String database;
  String measurement;
} InfluxTarget;

class SensorAddress {
  public:
    short device;
    short slot;

    inline SensorAddress() : device(0), slot(0) {}
    inline SensorAddress(short _device, short _slot) : device(_device), slot(_slot) {}

    inline bool operator==(const SensorAddress& sa) const { return sa.device==device && sa.slot==slot; }
    inline bool operator!=(const SensorAddress& sa) const { return sa.device!=device || sa.slot!=slot; }
    inline bool operator<(const SensorAddress& sa) const { return (sa.device==device) ? sa.slot<slot : sa.device<device; }
    inline bool operator>(const SensorAddress& sa) const { return (sa.device==device) ? sa.slot>slot : sa.device>device; }

    String toString() const;
};


#define F_BIT(x) (1<<x)

// Device Flags
#define DF_DISPLAY       F_BIT(0)
#define DF_BUS           F_BIT(1)
#define DF_I2C_BUS       (DF_BUS|F_BIT(2))
#define DF_SERIAL_BUS    (DF_BUS|F_BIT(3))


typedef enum SensorType {
  Invalid,
  Numeric,       // general numeric value
  Timestamp,     // a unix timestamp
  Milliseconds,  // a measurement in milliseconds
  Humidity,      // Relative Humidity (RH)
  Hygrometer,
  Temperature,   // Celcius or Farenheit
  HeatIndex,     // Celcius or Farenheit (adjusted with RH)
  Illuminance,   // Lux
  pH,
  ORP,           // unitless
  DissolvedOxygen, // mg/L
  Conductivity,  // microS/cm (Atlas Scientific)
  CO2,           // ppm
  Pressure,      // KPa?
  Flow,          // liquid or gas flow
  Altitude,      // meters or feet
  AirPressure,
  AirQuality,
  Voltage,
  Current,
  Watts,
  Motion,
  FirstSensorType=Humidity,   // types before this are considered generic such as configuration values
  LastSensorType=Motion
} SensorType;

const char* SensorTypeName(SensorType st);

typedef enum  {
  Offline,
  Degraded,
  Nominal
} DeviceState;

const char* DeviceStateName(DeviceState st);

struct _SensorInfo;
typedef struct _SensorInfo SensorInfo;

class Devices;
class Device;
class SensorReading;

struct _SensorInfo {
  String channelName;
  String driver;
  // influx target?
  uint8_t pin;  //todo: should be pinmap
  unsigned long updateFrequency;
};

typedef Device* (*DriverFactory)(SensorInfo* info);

typedef struct _DeviceDriverInfo {
  const char* name;
  const char* category;
  DriverFactory factory;
} DeviceDriverInfo;

class Devices {
  public:
    using WebServer = ::WebServer;
    
    //typedef Esp8266RestRequestHandler RestRequestHandler;
    //typedef Esp8266RestRequest RestRequest;  // RestRequestHandler::RequestType RestRequest;
    //typedef typename RestRequestHandler::HandlerType HandlerType;
    using Endpoints = typename RestRequestHandler::Endpoints;
    
    short slots;
    Device** devices;
    
    class ReadingIterator
    {
      public:
        SensorType sensorTypeFilter;
        char valueTypeFilter;
        unsigned long tsFrom, tsTo;
    
        Device* device;
        unsigned short slot;

        ReadingIterator& OfType(SensorType st);
        
        // returns an iterator that matches only specific time ranges
        ReadingIterator& TimeBetween(unsigned long from, unsigned long to);
        ReadingIterator& Before(unsigned long ts);
        ReadingIterator& After(unsigned long ts);
    
        SensorReading next();
        
      protected:
        Devices* manager;
        bool singleDevice;
        short deviceOrdinal;

        ReadingIterator(Devices* manager);

        // web handlers
        void httpGetDevices();
        
      friend class Devices;
      friend class DevicesRequestHandler;
    };

  protected:  
    /*class RequestHandler : public ::RequestHandler {
      public:
        RequestHandler(Devices* _owner) : owner(_owner) {}
        virtual bool canHandle(HTTPMethod method, String uri);
        virtual bool handle(ESP8266WebServer& server, HTTPMethod requestMethod, String requestUri);

        bool expectDevice(ESP8266WebServer& server, const char*& p, Device*& dev);
      private:
        Devices* owner;
    };*/
    
public:
    Devices(short maxDevices=32);
    ~Devices();

    void begin(WebServer& _http, NTPClient& client);

    // clear all devices
    void clearAll();

    short add(Device& dev);
    
    void remove(short deviceId);
    void remove(Device& dev);

    // find a device by id
    const Device& find(short deviceId) const;
    Device& find(short deviceId);

    // find a device by its alias
    const Device& find(String deviceAlias) const;
    Device& find(String deviceAlias);

    // find a reading by device:slot
    // for convenience, but if you are reading multiple values you should get the device ptr then read the slots (readings)
    SensorReading getReading(short deviceId, unsigned short slotId) const;
    SensorReading getReading(const SensorAddress& sa) const;

    // determine which devices need to interact with their hardware
    void handleUpdate();

    // iterate every reading available
    ReadingIterator forEach();

    // iterator readings from a deviceId
    ReadingIterator forEach(short deviceId);

    // get an iterator over a type of reading
    ReadingIterator forEach(SensorType st);

    // generate a file of all device and slot aliases
    String getAliasesFile();

    // parse a file of device and/or slot aliases and set where possible
    int parseAliasesFile(const char* aliases);

    // save alias file to SPIFFS fs
    bool saveAliasesFile(const char* aliases);

    // load aliases file from SPIFFS fs
    int restoreAliasesFile();

    // Web interface
    inline const WebServer& http() const { return *httpServer; }
    inline WebServer& http() { return *httpServer; }
    
    // Rest interface
    inline const RestRequestHandler& rest() const { return *restHandler; }
    inline RestRequestHandler& rest() { return *restHandler; }

    Endpoints::Node on(const char *endpoint_expression ) {
      return restHandler->on(endpoint_expression);   // add the rest (recursively)
    }

    // json interface
    void jsonGetDevices(JsonObject& root);
    void jsonForEachBySensorType(JsonObject& root, ReadingIterator& itr, bool detailedValues=true);

    static void registerDriver(const DeviceDriverInfo* driver);
    
  protected:
    short update_iterator;  // ordinal of next device update
    NTPClient* ntp;
    WebServer* httpServer;
    RestRequestHandler* restHandler;
    
    void alloc(short n);

    // do not allow copying
    Devices(const Devices& copy) = delete;
    Devices& operator=(const Devices& copy) = delete;

  protected:
    static const DeviceDriverInfo** drivers;
    static short driversSize;
    static short driversCount;

    void setupRestHandler();
    static const DeviceDriverInfo* findDriver(const char* name);
};

extern Devices DeviceManager;


#define VT_NULL  'n'      // null value
#define VT_CLEAR 'x'      // clear - indicates no measurement
#define VT_INVALID  '*'   // invalid reading
#define VT_FLOAT 'f'
#define VT_LONG  'l'
#define VT_INT   'l'
#define VT_BOOL  'b'

class SensorReading
{
  // todo: we should also add a String id and alias field (i.e. 1wire would have id as hex string, and alias as the temp probe name)
  public:
    SensorType sensorType;
    char valueType;
    unsigned long timestamp;
    union {
      float f;
      long l;
      bool b;
    };

    void clear();

    inline operator bool() const { return sensorType!=Invalid && valueType!=VT_INVALID; }
    
    String toString() const;

    void toJson(JsonObject& node, bool showType=true) const;
    void addTo(JsonArray& array) const;

  public:
    inline SensorReading() : sensorType(Numeric), valueType(VT_CLEAR), timestamp(millis()), l(0) {}
    inline SensorReading(SensorType st, char vt, long _l) : sensorType(st), valueType(vt), timestamp(millis()), l(_l) {}
    inline SensorReading(SensorType st, float _f) : sensorType(st), valueType('f'), timestamp(millis()), f(_f) {}
    inline SensorReading(SensorType st, double _f) : sensorType(st), valueType('f'), timestamp(millis()), f((float)_f) {}
    inline SensorReading(SensorType st, long _l) : sensorType(st), valueType('l'), timestamp(millis()), l(_l) {}
    inline SensorReading(SensorType st, int _i) : sensorType(st), valueType('i'), timestamp(millis()), l(_i) {}
    inline SensorReading(SensorType st, bool _b) : sensorType(st), valueType('b'), timestamp(millis()), b(_b) {}
};

extern SensorReading NullReading;
extern SensorReading InvalidReading;


class Device {
  public:
    short id;
    String alias;

  public:
    class Slot {
      public:
      String alias;
      SensorReading reading;
    };

  public:
    Device(short id, short _slots, unsigned long _updateInterval=1000, unsigned long _flags=0);
    Device(const Device& copy);
    virtual ~Device();
    Device& operator=(const Device& copy);

    virtual const char* getDriverName() const;
    
    operator bool() const;

    virtual void begin();

    inline unsigned long getFlags() const { return flags; }
    inline bool hasFlags(unsigned long f) const { return (flags & f)==f; }

    virtual DeviceState getState() const;
    
    // reset the device
    virtual void reset();

    // clear the readings
    virtual void clear();

    inline String getAlias() const { return alias; }

    // return the number of slots
    inline short slotCount() const { return slots; }

    String getSlotAlias(short slotIndex) const;
    void setSlotAlias(short slotIndex, String alias);

    short findSlotByAlias(String slotAlias) const;
    
    virtual void handleUpdate();

    void delay(unsigned long _delay);

    virtual bool isStale(unsigned long long _now=0) const;

    // read readings
    SensorReading& operator[](unsigned short slotIndex);

    // json interface
    void jsonGetReading(JsonObject& node, short slot);
    void jsonGetReadings(JsonObject& node);

    // standard rest methods
    int restInfo(RestRequest& request);
    
  protected:
    Devices* owner;
    unsigned short slots;
    Slot* readings;
    unsigned long flags;
    Devices::Endpoints endpoints;


    unsigned long updateInterval;
    unsigned long nextUpdate;

    DeviceState state;
    
    void alloc(unsigned short _slots);

    String prefixUri(const String& uri, short slot=-1) const;
    
// Web interface
    inline const Devices::WebServer& http() const { assert(owner); return owner->http(); }
    inline Devices::WebServer& http() { assert(owner); return owner->http(); }
    
    // Rest interface
    inline const RestRequestHandler& rest() const { assert(owner); return owner->rest(); }
    inline RestRequestHandler& rest() { assert(owner); return owner->rest(); }

    // same as http's on() methods except uri is prefixed with device specific path
    void onHttp(const String &uri, Devices::WebServer::THandlerFunction handler);
    void onHttp(const String &uri, HTTPMethod method, Devices::WebServer::THandlerFunction fn);
    void onHttp(const String &uri, HTTPMethod method, Devices::WebServer::THandlerFunction fn, Devices::WebServer::THandlerFunction ufn);

    Devices::Endpoints::Node on(const char *endpoint_expression) {
        assert(owner);
        return owner->on(endpoint_expression);   // add the rest (recursively)
    }
   
    // enable direct access to slots via http or mqqt
    // deprecated: void enableDirect(short slot, bool _get=true, bool _post=true);

    // http handlers
    // deprecated: void httpGetReading(short slot);
    // deprecated: void httpPostValue(short slot);

    void setOwner(Devices* owner);
    
    friend class Devices;
};

extern Device NullDevice;

void httpSend(Devices::WebServer& server, short responseCode, const JsonObject& json);
