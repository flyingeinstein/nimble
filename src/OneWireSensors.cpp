#include "OneWireSensors.h"
#include "ModuleManager.h"


OneWireSensor::OneWireSensor(short id, int _pin=0)
  : Module(id, 0, 1000), pin(_pin), oneWire(_pin), DS18B20(&oneWire)
{
  pinMode(pin, INPUT);

  // setup OneWire bus
  DS18B20.begin();
  //DS18B20.setResolution(sensorDeviceAddress, SENSOR_RESOLUTION);
}

OneWireSensor::OneWireSensor(const OneWireSensor& copy)
  : Module(copy), pin(copy.pin), oneWire(copy.oneWire), DS18B20(&oneWire)
{
}

OneWireSensor& OneWireSensor::operator=(const OneWireSensor& copy)
{
  Module::operator=(copy);
  pin = copy.pin;
  oneWire = copy.oneWire;
  DS18B20 = copy.DS18B20;
  return *this;
}

const char* OneWireSensor::getDriverName() const
{
  return "DallasOneWire";
}

int OneWireSensor::httpDevices(RestRequest& request)
{
  getDeviceInfo(request.response);
  return 200;
}

void OneWireSensor::begin()
{
  // todo: IMPLEMENT One Wire Sensor API
  #if 0
  std::function<int(RestRequest&)> func = [](RestRequest& request) {
    String s("Hello ");
    auto msg = request["msg"];
    if(msg.isString())
      s += msg.toString();
    else {
      s += '#';
      s += (long)msg;
    }
    request.response["reply"] = s;
    return 200;
  };

  on("/hello/:msg(string|integer)").GET( func );
  on("/onewire")
    .with(*this)
    .on("sensors")
      .GET(&OneWireSensor::httpDevices);
  
  //FakeHandler h = GET(std::bind(&handler_class::m, &c, std::placeholders::_1));
  //ModuleSet::HandlerType h = GET(std::bind(&OneWireSensor::httpDewices, this, std::placeholders::_1));
  //Rest::Handler< ModuleSet::RestRequest& > h = GET(std::bind(&OneWireSensor::httpDewices, this, std::placeholders::_1));
  //endpoints.on("dewices", GET(std::bind(&OneWireSensor::httpDewices, this, std::placeholders::_1)));
  #endif
}

const char* hex = "0123456789ABCDEF";

void OneWireSensor::getDeviceInfo(JsonObject& node)
{
  JsonArray jdevices = node.createNestedArray("devices");
  
  uint8_t count = DS18B20.getDS18Count();
  DeviceAddress devaddr;
  for (int i = 0; i < count; i++) {
    if (DS18B20.getAddress(devaddr, i)) {
      char addr[32];
      char *paddr = addr;
      for (int j = 0; j < 8; j++) {
        if (j > 0)
          *paddr++ = ':';
        *paddr++ = hex[ devaddr[j] / 16 ];
        *paddr++ = hex[ devaddr[j] % 16 ];
      }
      *paddr=0;

      jdevices.add(addr);
    }
  }
}

void OneWireSensor::handleUpdate()
{
  bool updateAliases = false;
  int good = 0, bad = 0;
  int count = DS18B20.getDS18Count();

  if(count <=0) {
    state = Nimble::Offline;
    return;
  }

  // ensure we have enough slots
  if(count > slotCount()) {
    alloc(count);
    updateAliases = true;
  }

  // read temperature sensors
  DS18B20.requestTemperatures();
  for (int i = 0; i < count; i++) {
    float f = DS18B20.getTempFByIndex(i);
    if (f > DEVICE_DISCONNECTED_F) {
      (*this)[i] = Nimble::SensorReading(Nimble::Temperature, f);
      good++;
    } else {
      (*this)[i] = Nimble::InvalidReading;
      bad++;
    }
  }
  
  state = (bad>0)
    ? (good>0)
      ? Nimble::Degraded
      : Nimble::Offline
    : Nimble::Nominal;

    if(updateAliases)
      Nimble::ModuleManager::Default.restoreAliasesFile();
}
