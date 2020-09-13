
#include "SensorReading.h"
#include "Module.h"


namespace Nimble {

SensorReading NullReading(Invalid, VT_NULL, 0);
SensorReading InvalidReading(Invalid, VT_INVALID, 0);

SensorReading::SensorReading(SensorType st)
  : sensorType(st)
{
  if(st == Numeric || (st >= FirstSensorType && st <= LastSensorType)) {
    valueType = VT_FLOAT;
    f = 0.0;
  } else {
    switch(st) {
      case Invalid:
        break;
      case SubModule:            // slot is a child-device (such as a device on a bus)
        valueType = VT_PTR;
        module = new Module(0, 0);
        break;
      case Config:               // Json configuration fragment
        break;
      case ErrorCode:            // indicates an error code, value is an integer
        valueType = VT_INT;
        l = 0;
        break;
      case Timestamp:            // a unix timestamp
      case Milliseconds:         // a measurement in milliseconds
        valueType = VT_INT;
        l = 0;
        break;
      case Json:                 // a Json fragment
        valueType = VT_PTR;
        json = new JsonObject();
        break;
      default:
        sensorType = Invalid;
        valueType = VT_NULL;
        break;
    }
  }
}

String SensorAddress::toString() const
{
  String s;
  s += device;
  s += ':';
  s += slot;
  return s;
}


void SensorReading::clear()
{
  if(valueType == VT_PTR) {
    switch(sensorType) {
      case SubModule: delete module; break;
      case Json: delete json; break;
      default: break;
    }
  }
  valueType = VT_CLEAR;
  timestamp = millis();
  l = 0;
}

String SensorReading::toString() const {
    switch(valueType) {
      case VT_INT:
      case VT_LONG: return String(l); break;
      case VT_FLOAT: return String(f); break;
      case VT_BOOL: return String(b ? "true":"false"); break;
      case VT_NULL:
      default:
        return "null"; break;
    }  
}

void SensorReading::addTo(JsonArray& arr) const
{
  switch(valueType) {
    case VT_INT:
    case VT_LONG: arr.add(l); break;
    case VT_FLOAT: arr.add(f); break;
    case VT_BOOL: arr.add(b); break;
    case VT_NULL:
    default:
      arr.add((char*)NULL); break;
  }    
}

void SensorReading::toJson(JsonObject& root, bool showType, bool showTimestamp) const {
  if(showType)
    root["type"] = SensorTypeName(sensorType);
    if(sensorType != Invalid) {
      if(showTimestamp)
        root["ts"] = timestamp;
      switch(valueType) {
        case VT_INT:
        case VT_LONG: root["value"] = l; break;
        case VT_FLOAT: if(!isnan(f)) root["value"] = f; break;
        case VT_BOOL: root["value"] = b; break;
        case VT_PTR: {
          if(module == nullptr) {
            root["value"] = nullptr;
          } else {
            String name = module->getDriverName();
            if(module->alias.length())
              name += " "+module->alias;
            root["value"] = name;
          }
        }
        case VT_NULL:
        default:
          root["value"] = (char*)NULL; break;
      }
  }
}

} // ns:Nimble
