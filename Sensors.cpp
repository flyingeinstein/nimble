
// include core Wiring API
#include <stdlib.h>
#include <HardwareSerial.h>

#include "Sensors.h"

const char* SensorTypeName(SensorType st)
{
  switch(st) {
    case Humidity: return "humidity";
    case Hygrometer: return "hygrometer";
    case Temperature: return "temperature";
    case Motion: return "motion";
    default: return "n/a";
  }
}

bool SensorReading::isStale(unsigned long long _now) const
{
  if(_now==0)
    _now = millis();
  return sensor!=NULL && (_now - sensor->updateFrequency) > timestamp;
}

String SensorReading::toString() const {
    switch(valueType) {
      default:
      case 'n': return "null"; break;
      case 'i':
      case 'l': return String(l); break;
      case 'f': return String(f); break;
      case 'b': return String(b ? "true":"false"); break;
    }  
}

Outputs::Outputs()
  : size(12), count(0), writePos(0), values(NULL) {
    Serial.print("Outputs.alloc constructor:"); Serial.println(size);
    values = (SensorReading*)calloc(size, sizeof(SensorReading));
}

Outputs::Outputs(const Outputs& copy) : size(copy.size), count(copy.count), writePos(0), values(NULL) {
    Serial.print("Outputs.alloc copy constructor:"); Serial.println(size);
    values = (SensorReading*)calloc(size, sizeof(SensorReading));
    memcpy(values, copy.values, copy.count*sizeof(SensorReading));
}

Outputs::~Outputs() {
  Serial.print("Outputs.alloc free:"); Serial.println(size);
  if(values) free(values);
}

Outputs& Outputs::operator=(const Outputs& copy) {
    size=copy.size;
    count=copy.count;
    writePos=copy.writePos;
    Serial.print("Outputs.alloc assignment:"); Serial.println(size);
    values = (SensorReading*)calloc(size, sizeof(SensorReading));
    memcpy(values, copy.values, copy.count*sizeof(SensorReading));
    return *this;
}

void Outputs::clear()
{
  memset(values, 0, count*sizeof(SensorReading));
  count=0;
  writePos=0;
}

void Outputs::resetWritePosition()
{
  writePos=0;
}

SensorReading* Outputs::read() const 
{
  return (writePos<count) 
    ? &values[writePos] 
    : NULL; 
}

void Outputs::write(const SensorReading& value) { 
  if(writePos>=size)
    alloc(size*1.25 + 1);
  if(writePos < count && values[writePos].sensor==value.sensor) {
    // update of reading
    values[writePos++] = value; 
  } else if(writePos < count) {
    // sensor is writing more values this time, insert the value
    if(count>=size)
      alloc(size*1.25 + 1);
    memmove(&values[writePos+1], &values[writePos], sizeof(values[0])*(count-writePos));  // make room for new reading
    count++;  // we expanded by 1
    values[writePos++] = value; // insert new reading
  } else {
    // new reading to append
    values[writePos] = value; 
    writePos++;
    count++;
  }
}

void Outputs::alloc(short n) {
  Serial.print("Outputs.alloc realloc:"); Serial.println(n);
  if(values) {
    values = (SensorReading*)realloc(values, n*sizeof(SensorReading));
    size = n;
  } else {
    values = (SensorReading*)calloc(size, sizeof(SensorReading));
  }
}
