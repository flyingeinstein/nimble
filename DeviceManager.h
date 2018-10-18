
#pragma once


#if (ARDUINO >= 100)
 #include <Arduino.h>
#else
 #include <WProgram.h>
 #include <pins_arduino.h>
#endif
#include <WString.h>

#include <NTPClient.h>

typedef struct _InfluxTarget {
  String database;
  String measurement;
} InfluxTarget;

typedef enum SensorType {
  Invalid,
  Numeric,       // general numeric value
  Humidity,      // Relative Humidity (RH)
  Hygrometer,
  Temperature,   // Celcius or Farenheit
  HeatIndex,     // Celcius or Farenheit (adjusted with RH)
  Illuminance,   // Lux
  pH,
  ORP,           // unitless
  Pressure,      // KPa?
  Altitude,      // meters or feet
  AirPressure,
  AirQuality,
  Voltage,
  Current,
  Watts,
  Motion
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
        
      friend class Devices;
    };
    
  public:
    Devices(short maxDevices=32);
    ~Devices();

    void begin(NTPClient& client);

    // clear all devices
    void clearAll();

    short add(Device* dev);

    // find a device by id
    const Device& find(short deviceId) const;
    Device& find(short deviceId);

    // find a reading by device:slot
    // for convenience, but if you are reading multiple values you should get the device ptr then read the slots (readings)
    SensorReading getReading(short deviceId, unsigned short slotId);

    // determine which devices need to interact with their hardware
    void handleUpdate();

    // iterate every reading available
    ReadingIterator forEach();

    // iterator readings from a deviceId
    ReadingIterator forEach(short deviceId);

    // get an iterator over a type of reading
    ReadingIterator forEach(SensorType st);

    static void registerDriver(const DeviceDriverInfo* driver);
    
  protected:
    short update_iterator;  // ordinal of next device update
    NTPClient* ntp;
    
    void alloc(short n);

    // do not allow copying
    Devices(const Devices& copy);
    Devices& operator=(const Devices& copy);

  protected:
    static const DeviceDriverInfo** drivers;
    static short driversSize;
    static short driversCount;
    
    static const DeviceDriverInfo* findDriver(const char* name);
};

extern Devices DeviceManager;

class Device {
  public:
    short id;

  public:
    Device(short id, short _slots, unsigned long _updateInterval=1000);
    Device(const Device& copy);
    virtual ~Device();
    Device& operator=(const Device& copy);

    inline bool isValid() const { return id>=0; }

    virtual DeviceState getState() const;
    
    // reset the device
    virtual void reset();

    // clear the readings
    virtual void clear();

    // return the number of slots
    inline short slotCount() const { return slots; }
    
    virtual void handleUpdate();

    virtual bool isStale(unsigned long long _now=0) const;

    // read readings
    SensorReading operator[](unsigned short slotIndex) const;

  protected:
    unsigned short slots;
    SensorReading* readings;

    unsigned long updateInterval;
    unsigned long nextUpdate;

    DeviceState state;
    
    void alloc(unsigned short _slots);

    friend class Devices;
};

extern Device NullDevice;

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
    unsigned long timestamp;
    char valueType;
    union {
      float f;
      long l;
      bool b;
    };

    void clear();

    inline operator bool() const { return sensorType!=Invalid && valueType!=VT_INVALID; }
    
    String toString() const;

  public:
    inline SensorReading() : sensorType(Numeric), valueType(VT_CLEAR), timestamp(millis()), l(0) {}
    inline SensorReading(SensorType st, char vt, long _l) : sensorType(st), valueType(vt), timestamp(millis()), l(_l) {}
    inline SensorReading(SensorType st, float _f) : sensorType(st), valueType('f'), timestamp(millis()), f(_f) {}
    inline SensorReading(SensorType st, long _l) : sensorType(st), valueType('l'), timestamp(millis()), l(_l) {}
    inline SensorReading(SensorType st, int _i) : sensorType(st), valueType('i'), timestamp(millis()), l(_i) {}
    inline SensorReading(SensorType st, bool _b) : sensorType(st), valueType('b'), timestamp(millis()), b(_b) {}
};

extern SensorReading NullReading;
extern SensorReading InvalidReading;
