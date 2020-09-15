
#include "Influx.h"
#include "ModuleManager.h"

using namespace Nimble;


Influx::Influx(short id)
  : Module(id, 0, 60000), influxUrl("http://kuba.lan:8086"), database(INFLUX_DATABASE), measurement(INFLUX_MEASUREMENT),
    data("4")
{
}

const char* Influx::getDriverName() const
{
    return "Influx";
}

int Influx::transmit()
{
    int httpCode = 200;

    if (WiFi.status() != WL_CONNECTED)
      return Rest::NetworkConnectTimeoutError;

    auto timestamp = unix_timestamp();
    if (timestamp < TIMESTAMP_MIN) // some time in 2017, before this code was built
      return Rest::ServiceUnavailable;

    unsigned long stopwatch = millis();
    String url = influxUrl;
    url += "/write?precision=s&db=";    // todo: precision in seconds?
    url += database;

    // tags
    int fields = 0;
    line = measurement;
    line += ",site=";
    line += hostname;

    auto& modules = ModuleManager::Default.modules();
    //auto& slot = modules.getSlotByAddress(line.c_str(), AnySensorType);
    auto& slot = modules.getSlot(2);
#if 1
    if(slot) {
      if(slot.reading.sensorType == SubModule) {
        Module* mod = slot.reading.module;
        
        // final tag is module alias
        line += ",module=";
        if(mod->alias.isEmpty())
          line += mod->getDriverName();
        else
          line += mod->alias;

        // tag/field seperator
        line += " ";

        // add slot readings as influx values
        int n = 0;
        mod->forEach([this, &n](const Slot& slot, void*) {
          if(n > 0)
            line += ',';
          
          line += slot.alias.isEmpty()
            ? SensorTypeName(slot.reading.sensorType)
            : slot.alias;
          
          line += '=';

          line += slot.reading.toString();

          n++;
          return true;
        });
      } else {
        line += "(reading)";

      }
    } else {
      line += "<*>";
    }
#endif

    // add timestamp required by influx
    line += " ";
    line += (unsigned long)timestamp;


#if 0
    WiFiClient client;
    HTTPClient http;
    http.begin(client, url);
    http.addHeader("Content-Type", "text/plain");
    httpCode = http.POST(line);
    //String payload = http.getString();
    //Serial.print("influx: ");
    //Serial.println(payload);
    http.end();
#endif

    stopwatch = millis() - stopwatch;
    Serial.print("influx post in ");
    Serial.print(stopwatch);
    Serial.print(" millis");

    //Serial.print(url);
    Serial.print(" => ");
    Serial.println(line);

    return httpCode;
}

void Influx::begin()
{
}

void Influx::handleUpdate()
{
  transmit();
  state = Nominal;
}

Rest::Endpoint Influx::delegate(Rest::Endpoint &p) 
{
    Module::delegate(p);

    if(auto config = p / "config") {
      config / Rest::GET([this](RestRequest& req) { 
          req.response["serverUrl"] = influxUrl;
          req.response["database"] = database;
          req.response["measurement"] = measurement;
          req.response["line"] = line;
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
      config / "data"
          / Rest::GET([this](RestRequest& req) { req.response["value"] = data; return Rest::OK; })
          / Rest::POST([this](RestRequest& req) { data = req.query("plain"); return Rest::OK; });
    }
    p / "line"
        / Rest::GET([this](RestRequest& req) { req.response["value"] = line; return Rest::OK; });

    return {};
}


#ifdef ENABLE_INFLUX
void sendToInflux()
{


  if (data.humidityPresent) {
    post += "humidity=";
    post += data.humidity;
    post += ",heatIndex=";
    post += data.heatIndex;
    fields += 2;
  }
  if (data.moisturePresent) {
    if (fields > 0)
      post += ",";
    post += "moisture=";
    post += data.moisture;
  }
  for (int i = 0; i < data.temperatureCount; i++) {
    if (fields > 0)
      post += ",";
    post += "t";
    post += i;
    post += "=";
    post += data.temperature[i];
  }

  // add timestamp required by influx
  post += " ";
  post += data.timestamp;

  if (true) {
    HTTPClient http;
    http.begin(url);
    http.addHeader("Content-Type", "text/plain");
    int httpCode = http.POST(post);
    //String payload = http.getString();
    //Serial.print("influx: ");
    //Serial.println(payload);
    http.end();

    stopwatch = millis() - stopwatch;
    Serial.print("influx post in ");
    Serial.print(stopwatch);
    Serial.print(" millis");
  }
  //Serial.print(url);
  Serial.print(" => ");
  Serial.println(post);
}
#endif

