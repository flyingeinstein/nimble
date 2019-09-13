# Development Plan

These items remain in development. As items are completed they are removed from this list.
* Adafruit Universal Sensor support 
** I would like my project to work more natively with the Universal Sensor support so that any Universal Sensor 
can be added to Nimble by just adding to platform.io config file.
* Support Wemos SSD1306 library
* Support ArduinoJson v5 and lower
* Make the device dependancies optional

## Version 1.0
* ModuleManager on() methods handle Rest URLs and handlers of ArduinoJson type
* Add Configuration API
  * Import/Export config via Json API
  * get/set via direct Rest API
  * works very much like Sensor values
  * loads drivers from configuration (not hard coded anymore)
* Get Influx working again
  * set config via Rest
     * function to create Line encodings
     * function to send the Line data

## Version 1.2
* I2C bus has its own ModuleSet object, and i2c buses are a child of I2C bus
     * i2c buses are no longer on the same ModuleManager as i2c bus
     * API now supports SensorAddresses with multiple levels (i.e 1:96:0 would typically be i2c-bus:ezo:pH)
     * might need to change Module class so slots are virtual, not backed by real SensorReading array. Can refactor, and make subclass implement a real array (maybe template based)
     * i2c bus can probe and create devices
     * ReadingIterator iterates within buses
* Atlas Scientific Probes (can already take readings, but no calibration done)
     * Support calibration
     * Support temperature compensation
     * Support slope
     * Support other status/info calls
* Add MQMTT support
* Module/Driver configuration is loaded/saved via Config API

## Version 1.4 - React App
* Add React web app and store in SPIFFS
* Can set aliases and configuration
* Visual KPIs and Dashboards
   
# IDEAS
* Convert NTP to be a sensor (time sensor). It has a few timestamps as readings. Also, other sensors can have a slot that outputs a timestamp, such as timestamp since last event.
* Perhaps Module class should just have a getReading() virtual method instead of an array. The sub-class can then decide to get the value on-request.
* Enable cloud support (either customer created or mine)
     ** brings all a users nimbles together
     ** enables sharing of nimbles with other users and groups (like weather data, power, gps) for crowd-sourced analytics
     ** simple docker image of a node.js app should do it

# Refactoring

### Refactor Device/Devices into Module and ModuleSet classes
All these sensors, devices and controllers could be just devices (renamed to modules) and if we also implement ModuleSet class 
derived from a module it can organize config into a subset of config modules. Also means influx module could be inserted multiple
times for multiple servers (but keeps the influx module simpler as a singleton). Root Http or Rest controller would parse a 
module/slot address into X:Y:Z:W, where each : identifies the next module ID, or finally the slot ID.
* [x] rename Device/Devices to Module/ModuleSet
* [x] move Module and related classes into Nimble namespace
* [x] move root rest/http handler out into new ModuleManager object (ModuleSet::rest/http will then be relative to the set's base path url)
* [x] write module/slot resolver function that takes a string and returns the ModuleSet, Module and (if present) the Slot number.
* [x] get/set alias file should be moved to ModuleManager

### ModuleSet is a Module, Module can contain sub-modules
* [x] both ModuleSet and Module has member slots (misnomer on ModuleSet)
* [x] Can we modify Slot to be the modules array
* [ ] Slot also has timestamp and alias, we can use these instead of storing inside Module

### Refactor Module rest handlers
* [ ] We intercept /api/device/[id_or_alias]
     * interceptor looks up Module* by id or alias
     * it checks if Module* has valid RestAPI endpoints, if so it delegates request to device's rest controller
     * otherwise, if no Module endpoints or endpoint not matched, it passes to the default rest device endpoints handler
* [ ] Refactor the default device rest controller into it's own class/module
* [ ] developers can extend the default Rest API controller or the specific one for a device
* [ ] also refactor other config/etc APIs into their own controller and attach to root one
     * Influx controller
     * Display controller (if we want to have multiple displays but only one programming protocol)

### Http handlers
[x] We can now attach handlers just like Rest ones but set expectation of a different contentType using the withContentType(...) 
method. We can even alllimitow application/json request data but also add support for other non-json mime types. 

### New classes
* [ ] implement influx module
* [ ] implement a statistics logger, possibly this should be external and centralized...influx?


### Event Log
Consolidate the Module::Statistics into a global event log instead. This will be simpler, more robust among various modules, and will get the code out of Module class.
* [ ] Event log will have members:
     * SensorAddress     - defines the x:y:z address of the module that generated the event
     * Severity (enum)   - debug, info, warning, error, fatal
     * Category (enum)   - Core, Config, Data, File, Web, ... (need to define, should this be module defined? do we really need this?)
     * Source (id)       - defined by the module
     * detail (optional) - detailed text
* [ ] Module defined dictionaries such as Source should be stored in the Module factory and loaded from filesystem. We could have the factory provide a dictionary interface for more than just events.
* [ ] API to retrieve log events and dictionaries.
* [ ] look at central event logging systems out there, there must be something that we can just send events too and forget  (geekflare, central logging)[https://geekflare.com/open-source-centralized-logging]
* [ ] influx, mqmtt or SNMP or other module can be used to send the log to a central system (and then clear it?)

### Cleaner interfaces
* [ ] possibly we dont even need ModuleSet anymore
** A Module has slots as sub-modules and ModuleSet is now implemented by Module, so its not really differentiable from ModuleSet
* [ ] improve forEach()
** The new forEach(lamda...) is really slick. I bet we can convert ReadingIterator over to that
** ReadingIterator has timeBetween and other filter methods, keep this support
** Possibly forEach() could return ReadingIterator and that class would have a call(lambda) method that is like the forEach(lambda)
** Move ReadingIterator into its own class though 
* [ ] Possibly SensorReading needs to change names since it may be a SubModule now