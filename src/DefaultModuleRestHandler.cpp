
#include "DefaultModuleRestHandler.h"

namespace Nimble {

DefaultModuleRestHandler::DefaultModuleRestHandler()
{
  ModuleSet& _modules = ModuleManager::Default.modules();

  std::function<int(RestRequest&)> func = [&_modules](RestRequest& request) {
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

  // resolves a device number to a Module object
  std::function<Module*(Rest::UriRequest&)> device_resolver = [this,&_modules](Rest::UriRequest& request) -> Module* {
    Rest::Argument req_dev = request["devaddr"];
    Module& dev = _modules[ req_dev ];

    // check for NOT FOUND
    if (&dev == &NullModule) {
      request.abort(404);
      return nullptr;
    }

    return &dev;
  };

  std::function<const Module*(Rest::UriRequest&)> const_device_resolver = [this,&_modules](Rest::UriRequest& request) -> const Module* {
    Rest::Argument req_dev = request["devaddr"];
    Module& dev = _modules[ req_dev ];
         
    // check for NOT FOUND
    if (&dev == &NullModule) {
      request.abort(404);
      return nullptr;
    }

    return &dev;
  };

  auto device_api_resolver = [this,&_modules](Rest::ParserState& state) -> Endpoints::Handler {
    Rest::Argument req_dev = state.request.args["devaddr"];
    Module* dev = nullptr;

    if( req_dev.isString() ) {
      auto slot = _modules.slotByAddress( (const char*)req_dev );
      if(slot.reading.sensorType == SubModule)
        dev = slot.reading.module;
    } else if (req_dev.isNumber()) {
      short module_id = (int)req_dev;
      if(module_id < _modules.slotCount())
        dev = &_modules[ module_id ];
    }

    if(dev == nullptr || dev == &NullModule) {
      // slot or module not found
      state.request.status = 404;
      return Endpoints::Handler();
    }

    // device found, see of the device has an API extension
    typename Endpoints::Node rhs_node;

    if(dev->hasEndpoints()) {
      // try to resolve the endpoint using the device's API extension
      rhs_node = dev->endpoints()->getRoot();
    } else {
      // device has no API extension, so use the default one
      // todo: setup the device default API controller
      rhs_node = this->_defaultModuleEndpoints.getRoot();
    }

    if(rhs_node) {
      Rest::ParserState rhs_state(state); // copy state and parse remaining
      typename Endpoints::Handler handler = rhs_node.resolve(rhs_state);
      if (handler!=nullptr) {
          state = rhs_state;      // we resolved a handler, so we modify the lhs request with rhs (which may have extra args)
          return handler;   
      }
    }
  
    // device or handler not found
    return Endpoints::Handler();
  };

  // Playground
  on("echo/:msg(string|integer)").GET( func );
  
  // ModuleSet API
  on("devices")
    .GET([this,&_modules](RestRequest& request) { jsonGetModules(_modules, request.response); return 200; });
  
  // delegate device API requests to the Module or the default device API controller
  on("device/:devaddr(string|integer)")
    .otherwise(device_api_resolver);

  _defaultModuleEndpoints.getRoot()
    .with(const_device_resolver)
    .GET(&Module::restDetail)
    .GET("status", &Module::restStatus)
    .GET("slots", &Module::restSlots)
    .GET("statistics", &Module::restStatistics);
  //on("/api/slot/:slotaddr(string|integer)")
  //  .otherwise(slot_api_resolver);
}

bool DefaultModuleRestHandler::hasEndpoints() const {
    return true; 
}

const Endpoints* DefaultModuleRestHandler::endpoints() const 
{
   return &_endpoints; 
}

Endpoints* DefaultModuleRestHandler::endpoints() 
{
   return &_endpoints;
}


void DefaultModuleRestHandler::jsonGetModules(ModuleSet& modset, JsonObject &root)
{
  // list all devices
  JsonArray devs = root.createNestedArray("devices");
  modset.forEach( [&devs](const Module& module, void* userData) {

    // get the slot
    JsonObject jdev = devs.createNestedObject();
    const char* driverName = module.getDriverName();

    // id
    jdev["id"] = module.id;

    // driver name
    if(driverName!=NULL)
        jdev["driver"] = driverName;

    // alias
    if(module.alias.length())
        jdev["alias"] = module.alias;

    // device slot metadata
    JsonArray jslots = jdev.createNestedArray("slots");
    for(int j=0, _j = module.slotCount(); j<_j; j++) {
        SensorReading r = module[j];
        if(r) {
            JsonObject jslot = jslots.createNestedObject();

            // slot alias
            String alias = module.getSlotAlias(j);
            if(alias.length())
                jslot["alias"] = alias;

            // slot sensor type
            jslot["type"] = SensorTypeName(r.sensorType);
        }
    }

    return true;
  });
}

void DefaultModuleRestHandler::jsonForEachBySensorType(JsonObject& root, ModuleSet::ReadingIterator& itr, bool detailedValues)
{
  SensorReading r;
  JsonArray groups[LastSensorType];
  //memset(groups, 0, sizeof(groups));
  
  while( (r = itr.next()) ) {
    if(r.sensorType < FirstSensorType || r.sensorType >= LastSensorType)
      continue; // guard array bounds
    
    // get the group for this sensor
    JsonArray jgroup = groups[r.sensorType];
    if(jgroup.isNull())
      jgroup = groups[r.sensorType] = root.createNestedArray(SensorTypeName(r.sensorType));
    
    // add this sensor value
    if(detailedValues) {
      JsonObject jr = jgroup.createNestedObject();
      jr["address"] = SensorAddress(itr.device->id, itr.slot).toString();
      r.toJson(jr, false);
    } else {
      r.addTo(jgroup);
    }
  }
}


}  // ns:Nimble
