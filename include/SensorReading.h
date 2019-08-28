
#pragma once

#include "NimbleConfig.h"


#define F_BIT(x) (1<<x)

#define VT_NULL  'n'      // null value
#define VT_CLEAR 'x'      // clear - indicates no measurement
#define VT_INVALID  '*'   // invalid reading
#define VT_FLOAT 'f'
#define VT_LONG  'l'
#define VT_INT   'l'
#define VT_BOOL  'b'


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

    void toJson(JsonObject& node, bool showType=true, bool showTimestamp=true) const;
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
