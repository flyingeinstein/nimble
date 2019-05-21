#include "DHTSensor.h"


DHTSensor::DHTSensor(short id, uint8_t _pin, uint8_t _type)
  : Device(id, 3, 2500), dht(_pin, _type), attempts(0), backoffDelay(10000)
{
  dht.begin();
}

DHTSensor::DHTSensor(const DHTSensor& copy)
  : Device(copy), dht(copy.dht), attempts(copy.attempts), backoffDelay(copy.backoffDelay)
{
}

DHTSensor& DHTSensor::operator=(const DHTSensor& copy)
{
  Device::operator=(copy);
  dht = copy.dht;
  attempts = copy.attempts;
  backoffDelay = copy.backoffDelay;
  return *this;
}

const char* DHTSensor::getDriverName() const
{
  return "DHT";
}

void DHTSensor::handleUpdate()
{
  float h = dht.readHumidity();
  // Read temperature as Fahrenheit (isFahrenheit = true)
  float f = dht.readTemperature(true);

  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(f)) {
    ++attempts;
    if (attempts > 12) {
      state = Offline;
      nextUpdate = millis() + 3*backoffDelay;  // now we defer the next read attempt for 3 times the back-off period
    } else if (attempts > 3) {
      state = Degraded;  // skip humidity
      nextUpdate = millis() + backoffDelay;  // now we defer the next read attempt for a back-off
    }
  } else {
    state = Nominal;
    attempts =0;
  }

  (*this)[0] = SensorReading(Humidity, h);
  (*this)[1] = SensorReading(Temperature, f);
  (*this)[2] = SensorReading(HeatIndex, dht.computeHeatIndex(f, h));
}
