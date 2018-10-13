
#pragma once

#if (ARDUINO >= 100)
 #include <Arduino.h>
#else
 #include <WProgram.h>
 #include <pins_arduino.h>
#endif
#include <WString.h>

typedef struct _InfluxTarget {
  String database;
  String measurement;
} InfluxTarget;

struct _SensorInfo;
typedef struct _SensorInfo SensorInfo;

class SensorReading;

typedef enum {
  Humidity,
  Hygrometer,
  Temperature,
  Motion
} SensorType;

const char* SensorTypeName(SensorType st);

class Outputs {
  public:
    short size;
    short count;
    short writePos;
    SensorReading* values;

  public:
    Outputs();
    Outputs(const Outputs& copy);
    ~Outputs();
    Outputs& operator=(const Outputs& copy);
    
    void clear();
    void reset();

    // write readings at the current position
    void write(const SensorReading& value);
    
    // read readings
    SensorReading* read() const;

    void resetWritePosition();

  protected:
    void alloc(short n);
};

typedef short (*sensor_read_callback)(const SensorInfo& sensor, Outputs& outputs);


struct _SensorInfo {
  SensorType type;
  String name;
  InfluxTarget target;
  uint8_t pin;  //todo: should be pinmap
  unsigned long updateFrequency;
  sensor_read_callback read;
};

#define VT_NULL  'n'      // null value
#define VT_INVALID  '*'   // invalid reading
#define VT_FLOAT 'f'
#define VT_LONG  'l'
#define VT_INT   'l'
#define VT_BOOL  'b'

class SensorReading
{
  public:
    SensorType sensorType;
    const SensorInfo* sensor;
    unsigned long timestamp;
    char valueType;
    union {
      float f;
      long l;
      bool b;
    };

    String toString() const;

    bool isStale(unsigned long long _now=0) const;

  public:
    inline SensorReading(const SensorInfo& s, float _f) : sensor(&s), sensorType(s.type), valueType('f'), timestamp(millis()), f(_f) {}
    inline SensorReading(const SensorInfo& s, long _l) : sensor(&s), sensorType(s.type), valueType('l'), timestamp(millis()), l(_l) {}
    inline SensorReading(const SensorInfo& s, int _i) : sensor(&s), sensorType(s.type), valueType('i'), timestamp(millis()), l(_i) {}
    inline SensorReading(const SensorInfo& s, bool _b) : sensor(&s), sensorType(s.type), valueType('b'), timestamp(millis()), b(_b) {}
};
