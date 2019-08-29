/**
 * @file AnalogPin.h
 * @author Colin F. MacKenzie (nospam2@colinmackenzie.net)
 * @brief Analog pin as a device
 * @version 0.1
 * @date 2019-08-28
 * 
 * @copyright Copyright (c) 2019
 * 
 */
#pragma once


#include "NimbleAPI.h"

/**
 * @brief Default converter simply passes on the raw value
 * 
 */
struct NoConversion {
  inline short operator()(short value) const { return value; }
};


/**
 * @brief Represents an analog pin as a device.
 * The pin has a single slot with the analog value. When subclassed can be provided a template argument
 * to act as conversion from raw analog value to a sensor reading.
 * 
 * @tparam Converter=NoConversion 
 */
template<class Converter=NoConversion>
class AnalogPin : public Device
{
  public:
    AnalogPin(short id, SensorType _pinType, int _pin=0)
      : Device(id, 1, 250), pinType(_pinType), pin(_pin)
    {
        pinMode(pin, INPUT);
    }
    
    AnalogPin(const AnalogPin& copy)
      : Device(copy), pinType(copy.pinType), pin(copy.pin)
    {
    }
    
    AnalogPin& operator=(const AnalogPin& copy)
    {
      Device::operator=(copy);
      pinType = copy.pinType;
      pin = copy.pin;
      return *this;
    }

    /// @brief returns the string "adc" (Analog to Digitan Converter)
    virtual const char* getDriverName() const { return "adc"; }

    /// @brief Reads the analog pin into slot[0]
    virtual void handleUpdate()
    {
      short v = analogRead(pin);
      decltype(converter()) v2 = converter(v);
      readings[0] = SensorReading(pinType, v2);
      state = Nominal;
    }

  protected:
    Converter converter;
    SensorType pinType;
    int pin;
};

/// @brief Converts a raw analog value to a moisture relative value between 0 and 2, with 1.0 being typical.
struct MoistureConversion {
  inline float operator()(short value) const { return (1024 - value)/512.0; }
};

/// @brief A capacitive moisture sensor
/// This also represents an example of an AnalogPin with a value conversion function supplied.
typedef AnalogPin<MoistureConversion> MoistureSensor;
