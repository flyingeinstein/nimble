
#include "SensorReading.h"
#include "Module.h"


namespace Nimble {

SensorReading NullReading(Invalid, VT_NULL, 0);
SensorReading InvalidReading(Invalid, VT_INVALID, 0);


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
  valueType = VT_CLEAR;
  timestamp = millis();
  l = 0;
}

String SensorReading::toString() const {
    switch(valueType) {
      case 'i':
      case 'l': return String(l); break;
      case 'f': return String(f); break;
      case 'b': return String(b ? "true":"false"); break;
      case 'n':
      default:
        return "null"; break;
    }  
}

void SensorReading::addTo(JsonArray& arr) const
{
  switch(valueType) {
    case 'i':
    case 'l': arr.add(l); break;
    case 'f': arr.add(f); break;
    case 'b': arr.add(b); break;
    case 'n':
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
        case 'i':
        case 'l': root["value"] = l; break;
        case 'f': if(!isnan(f)) root["value"] = f; break;
        case 'b': root["value"] = b; break;
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
        case 'n':
        default:
          root["value"] = (char*)NULL; break;
      }
  }
}

} // ns:Nimble
