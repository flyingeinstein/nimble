#include "OneWireSensors.h"


OneWireSensor::OneWireSensor(short id, int _pin=0)
  : Device(id, 0, 1000), pin(_pin), oneWire(_pin), DS18B20(&oneWire)
{
  pinMode(pin, INPUT);

  // setup OneWire bus
  DS18B20.begin();
  //DS18B20.setResolution(sensorDeviceAddress, SENSOR_RESOLUTION);
}

OneWireSensor::OneWireSensor(const OneWireSensor& copy)
  : Device(copy), pin(copy.pin), oneWire(copy.oneWire), DS18B20(&oneWire)
{
}

OneWireSensor& OneWireSensor::operator=(const OneWireSensor& copy)
{
  Device::operator=(copy);
  pin = copy.pin;
  oneWire = copy.oneWire;
  DS18B20 = copy.DS18B20;
  return *this;
}

void OneWireSensor::handleUpdate()
{
  int good = 0, bad = 0;
  int count = DS18B20.getDS18Count();

  if(count <=0) {
    state = Offline;
    return;
  }

  // ensure we have enough slots
  if(count > slotCount())
    alloc(count);

  // read temperature sensors
  DS18B20.requestTemperatures();
  for (int i = 0; i < count; i++) {
    float f = DS18B20.getTempFByIndex(i);
    if (f > DEVICE_DISCONNECTED_F) {
      readings[i] = SensorReading(Temperature, f);
      good++;
    } else {
      readings[i] = InvalidReading;
      bad++;
    }
  }
  
  state = (bad>0)
    ? (good>0)
      ? Degraded
      : Offline
    : Nominal;
}
