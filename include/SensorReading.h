/**
 * @file SensorReading.h
 * @author Colin F. MacKenzie (nospam2@colinmackenzie.net)
 * @brief Holds sensor readings of any primitive type such as integers, strings, reals and more.
 * @version 0.1
 * @date 2019-08-28
 * 
 * @copyright Copyright (c) 2019
 * 
 */

#pragma once

#include "NimbleConfig.h"
#include "NimbleEnum.h"


#define F_BIT(x) (1<<x)

//@{
/// @name Sensor Reading Primitive Types
#define VT_NULL     'n'      // null value
#define VT_CLEAR    'x'      // clear - indicates no measurement
#define VT_INVALID  '*'      // invalid reading
#define VT_FLOAT    'f'
#define VT_LONG     'l'
#define VT_INT      'l'
#define VT_BOOL     'b'
//@}

/**
 * @brief Indicates what sensor recorded the reading, as a device:slot address.
 * 
 */
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

/**
 * @brief Holds sensor readings of any primitive type such as integers, strings, reals and more.
 * 
 */
class SensorReading
{
  // todo: we should also add a String id and alias field (i.e. 1wire would have id as hex string, and alias as the temp probe name)
  public:
    SensorType sensorType;      // the class of sensor
    char valueType;             // what primitive type is store in the reading (see VT_xxxx defines)
    unsigned long timestamp;    // when the reading was first recorded
    union {
      float f;
      long l;
      bool b;
    };

    /**
     * @brief Clear the reading, setting the value to VT_NULL
     * 
     */
    void clear();

    /**
     * @brief returns true if a reading is valid
     * 
     * @return true 
     * @return false 
     */
    inline operator bool() const { return sensorType!=Invalid && valueType!=VT_INVALID; }
    
    /**
     * @brief Convert the reading to a printable text
     * 
     * @return String 
     */
    String toString() const;

    /**
     * @brief Converts the sensor reading to a Json object
     * 
     * @param node details of the reading will be added to this existing JsonObject node
     * @param showType set if you want the sensor type to be added to the Json output node
     * @param showTimestamp  set if you want the timestamp of when the reading was recorded to be added to the Json output node
     */
    void toJson(JsonObject& node, bool showType=true, bool showTimestamp=true) const;

    /**
     * @brief Uses toJson() to create and add the reading to the given Json array.
     * 
     * @param array The array to receive the new sensor reading Json object
     */
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
