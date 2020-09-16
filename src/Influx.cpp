
#include "Influx.h"
#include "ModuleManager.h"

using namespace Nimble;

//#define PRINT_STATS

Influx::Influx(short id)
  : Module(id, 0, 60000), influxUrl("http://kuba.lan:8086"), database(INFLUX_DATABASE), measurement(INFLUX_MEASUREMENT), enable(false)
{
  targets.emplace_back(Target(4, {0, 1}));
  targets.emplace_back(Target(5, {1}));
}

const char* Influx::getDriverName() const
{
    return "Influx";
}

void concatSlot(String& s, const Module::Slot& slot, char sep) {
  s += sep;
  
  s += slot.alias.isEmpty()
    ? SensorTypeName(slot.reading.sensorType)
    : slot.alias;

  s += '=';

  s += slot.reading.toString();
}

String Influx::format(const Influx::Target& target) const {
  // tags
  String s = target.measurement.isEmpty()
    ? measurement
    : target.measurement;

  s += ",site=";
  s += hostname;

  auto& modules = ModuleManager::Default.modules();
  
  if(auto& module = modules.getModule(target.module)) {
    // final tag is module alias
    s += ",module=";
    if(module.alias.isEmpty())
      s += module.getDriverName();
    else
      s += module.alias;

    // add slot readings as influx values
    int n = 0;
    if(target.slots.empty()) {
      // output all slot values
      module.forEach([&s, &n](const Slot& slot, void*) {
        if(slot.reading) {
          concatSlot(s, slot, n ? ',' : ' ');
          n++;
        }
        return true;
      });
    } else {
      for(int sid: target.slots) {
        const auto& slot = module.getSlot(sid);
        if(slot && slot.reading) {
          concatSlot(s, slot, n ? ',' : ' ');
          n++;            
        }
      }
    }

    // if no values were written, return no line
    if(n==0)
      return "";

  } else
    return "";

  // add timestamp required by influx
  s += ' ';
  s += (unsigned long)lastUpdate;
  s += '\n';
  
  return s;
}

int Influx::transmit()
{
    int httpCode = 200;

    if (WiFi.status() != WL_CONNECTED)
      return Rest::NetworkConnectTimeoutError;

    auto timestamp = unix_timestamp();
    if (timestamp < TIMESTAMP_MIN) // some time in 2017, before this code was built
      return Rest::ServiceUnavailable;
    
    lastUpdate = timestamp;
    
#if defined(PRINT_STATS)
    unsigned long stopwatch = millis();
#endif

    String url = influxUrl;
    url += "/write?precision=s&db=";    // todo: precision in seconds?
    url += database;

    line = "";
    for(const auto& target: targets) {
      line += format(target);
    }

    if(enable) {
      WiFiClient client;
      HTTPClient http;
      http.begin(client, url);
      http.addHeader("Content-Type", "text/plain");
      httpCode = http.POST(line);
      //String payload = http.getString();
      //Serial.print("influx: ");
      //Serial.println(payload);
      http.end();
    }

#if defined(PRINT_STATS)
    stopwatch = millis() - stopwatch;
    Serial.print("influx post in ");
    Serial.print(stopwatch);
    Serial.print(" millis");

    //Serial.print(url);
    Serial.print(" => ");
    Serial.println(line);
#endif

    return httpCode;
}

void Influx::begin()
{
}

void Influx::handleUpdate()
{
  auto code = transmit();
  state = (code == Rest::OK)
    ? Nominal
    : Offline;
}

int Influx::dataToJson(JsonObject& target) {
  auto elements = target.createNestedArray("data");
  for(auto& t : targets) {
    auto el = elements.createNestedObject();
    t.toJson(el, ModuleManager::Default.modules());
  }
  return Rest::OK;
}

int Influx::dataFromJson(JsonObject& source) {
#if 0
  if(!source.is<JsonArray>()) {
    // element should be an array, but we can check if there is a data array
    if(source.is<JsonObject>() && (auto _data = source["data"])) {
      // we found a data element
      return dataFromJson(_data);
    } else
      return Rest::BadRequest;
  }
  JsonArray jsrc = source.as<JsonArray>();
#else
  JsonArray jsrc = source["data"].as<JsonArray>();;
  if(jsrc.isNull()) {
    Serial.println("no data element");
    Serial.println(source.size());
    return Rest::BadRequest;
  }
#endif

  std::vector<Target> _targets;
  for(JsonVariant v : jsrc) {
    if(v.is<JsonObject>()) {
      _targets.emplace_back(v.as<JsonObject>());
    } else
      return Rest::BadRequest;
  }
  targets = _targets; // replace current config
  return Rest::OK;
}

Rest::Endpoint Influx::delegate(Rest::Endpoint &p) 
{
    Module::delegate(p);

    if(auto config = p / "config") {
      config / Rest::GET([this](RestRequest& req) { 
          req.response["serverUrl"] = influxUrl;
          req.response["database"] = database;
          req.response["measurement"] = measurement;
          req.response["enable"] = enable ? "true" : "false";
          req.response["line"] = line;
          dataToJson(req.response);
          return Rest::OK;
      });

      // todo: we should be able to add Rest::VARIABLE or something and have Endpoint 
      // do the GET/POST for us, plus have it all wrap up into *whole* config call above.
      config / "serverUrl" 
          / Rest::GET([this](RestRequest& req) { req.response["value"] = influxUrl; return Rest::OK; })
          / Rest::POST([this](RestRequest& req) { influxUrl = req.query("plain"); return Rest::OK; });
      config / "database"
          / Rest::GET([this](RestRequest& req) { req.response["value"] = database; return Rest::OK; })
          / Rest::POST([this](RestRequest& req) { database = req.query("plain"); return Rest::OK; });
      config / "measurement"
          / Rest::GET([this](RestRequest& req) { req.response["value"] = measurement; return Rest::OK; })
          / Rest::POST([this](RestRequest& req) { measurement = req.query("plain"); return Rest::OK; });
      config / "enable"
          / Rest::GET([this](RestRequest& req) { req.response["value"] = enable ? "true" : "false"; return Rest::OK; })
          / Rest::POST([this](RestRequest& req) { enable = req.query("plain") == "true"; return Rest::OK; });
      config / "data"
          / Rest::GET([this](RestRequest& req) {
              return dataToJson(req.response);
            })
          / Rest::POST([this](RestRequest& req) {
              auto body = req.body.as<JsonObject>();
              return dataFromJson(body);
            });
    }

    return {};
}



Influx::Target::Target() 
  : module(-1)
{
}

Influx::Target::Target(int _module, std::initializer_list<short>&& _slots, const char* _measurement)
  : module(_module), slots(_slots)
{
  if(_measurement)
    measurement = _measurement;
}

Influx::Target::Target(JsonObject json) 
  : Target()
{
  if(!json.isNull()) {
    auto _measurement = json["measurement"];
    auto _module = json["module"];

    if(!_measurement.isNull() && _measurement.is<char*>())
      measurement = _measurement.as<String>();
    if(!_module.isNull() && _module.is<int>())
      module = _module;

    auto _slots = json["slots"];
    if(!_slots.isNull()) {
      if(_slots.is<int>()) {
        slots.push_back(_slots);
      } else if(_slots.is<JsonArray>()) {
        JsonArray sarr = _slots.as<JsonArray>();
        for(JsonVariant v : sarr) {
          if(v.is<int>())
            slots.push_back(v.as<int>());
        }
      }
    }
  }
}

int Influx::Target::toJson(JsonObject& target, ModuleSet& modules) const {
  if(!measurement.isEmpty())
    target["measurement"] = measurement;
  
  Module* mod = nullptr;
  target["module"] = module;
  if((mod = resolve(modules))) {
    target["driver"] = mod->getDriverName();
    if(!mod->alias.isEmpty())
      target["alias"] = mod->alias;
  }

  auto _slots = target.createNestedArray("slots");
  auto _slotnames = mod ? target.createNestedArray("names") : JsonArray();
  for(auto& sid : slots) {
    _slots.add(sid);
    if(mod) {
      auto& slot = mod->getSlot(sid);
      _slotnames.add(slot.alias.isEmpty()
        ? Nimble::SensorTypeName(slot.reading.sensorType)
        : slot.alias
      );
    }
  }
  return 0;
}

Module* Influx::Target::resolve(ModuleSet& modules) const {
  auto& mod = modules.getModule(module);
  return mod 
    ? &mod
    : nullptr;
}


