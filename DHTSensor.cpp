#include "DHTSensor.h"


DHTSensor::DHTSensor(short id, int _pin)
  : Device(id, 3, 1000), dht(_pin, DHTTYPE), backoffDelay(10000)
{
  pinMode(_pin, INPUT);
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
    return;
  }

  (*this)[0] = SensorReading(Humidity, h);
  (*this)[1] = SensorReading(Temperature, f);
  (*this)[2] = SensorReading(HeatIndex, dht.computeHeatIndex(f, h));
  
  state = Nominal;
}
