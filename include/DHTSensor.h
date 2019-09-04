
#pragma once


#include "NimbleAPI.h"

#include <DHT.h>

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


class DHTSensor : public Nimble::Module
{
  public:
    DHTSensor(short id, uint8_t _pin, uint8_t _type);
    DHTSensor(const DHTSensor& copy);
    DHTSensor& operator=(const DHTSensor& copy);

    virtual const char* getDriverName() const;

    virtual void handleUpdate();

  public:
    DHT dht;
    short attempts;
    unsigned long backoffDelay;   // delay if sensor has failed to read more than 3 times in a row
};
