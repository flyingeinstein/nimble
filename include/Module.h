/**
 * @file Module.h
 * @author Colin F. MacKenzie (nospam2@colinmackenzie.net)
 * @brief Base class for devices
 * @version 0.1
 * @date 2019-08-27
 * 
 * @copyright Copyright (c) 2019
 * 
 */
#pragma once

#include "NimbleConfig.h"
#include "SensorReading.h"

// Module Flags
#define MF_DISPLAY       F_BIT(0)             /// Module is some sort of display device
#define MF_BUS           F_BIT(1)             /// Module is a bus containing sub-devices
#define MF_I2C_BUS       (MF_BUS|F_BIT(2))    /// Module is an i2c bus (therefor DF_BUS flag will also be set)
#define MF_SERIAL_BUS    (MF_BUS|F_BIT(3))    /// Module is a serial bus (therefor DF_BUS flag will also be set)

namespace Nimble {

class Module;
class ModuleSet;


/**
 * @brief Base class for all devices.
 * This class provides base functionality for all devices. It provides to a base class
 * Rest and Http endpoint services and sensor/slot management.
 * 
 */
class Module {
    friend class ModuleSet;
  public:
    short id;
    String alias;

  public:
    /// flags controlling toJson output detail
    typedef enum : int {
      JsonSlots        = 1,
      JsonStatistics   = 2,
      JsonDefault      = JsonSlots
    } JsonFlags;
  
    /**
     * @brief Contains a single measurement or sensor reading.
     * A device can contain one or more slots but the number of slots should stay constant. For each update,
     * the device should measure and update the sensors associated with each slot. A sensor, such as DHT 
     * humidity sensors, may update many slots in one call such as humidity, temperature and "feels-like" 
     * temperature.
     * 
     */
    class Slot {
      public:
        String alias;             /// slot alias (may be blank if not set)
        SensorReading reading;    /// the most recent sensor reading

        /// @brief Constructs a special Slot with an ErrorCode
        /// The slot contains a reading of type ErrorCode with value as errorCode. Optionally the Slot alias
        /// member will bet set to the errstr.
        static Slot makeError(long errorCode, String errstr);
    };

    /**
     * @brief Collects general operating statistics for a device.
     * 
     */
    class Statistics {
      public:
        int updates;      /// number of requests for new measurements
        
        struct {
          int bus;        /// errors communicating with the sensor
          int sensing;    /// errors reported by sensor (failure to sense)
        } errors;

        inline Statistics() : updates(0) { memset(&errors, 0, sizeof(errors)); }
        
        /// Serialize this statistics object to a JsonObject
        void toJson(JsonObject& target) const;
    };

  public:
    using SlotCallback = std::function< bool(Module::Slot&, void*) >;
    using ReadingCallback = std::function< bool(SensorReading&, void*) >;
    using ModuleCallback = std::function< bool(Module&, void*) >;


    /**
     * @brief Construct a new Module object
     * 
     * @param id The device id.
     * @param _slots The number of slots for measurements this device has.
     * @param _updateInterval The number of milliseconds between successive measurements. The framework will call Module::update() each time this timer expires.
     * @param _flags Module flags; such as indicating if the device is a BUS with sub-devices.
     */
    Module(short id, short _slots, unsigned long _updateInterval=1000, unsigned long _flags=0);

    /**
     * @brief Construct a new Module object
     * 
     * @param copy Will make a copy of this device.
     */
    Module(const Module& copy);

    virtual ~Module();

    Module& operator=(const Module& copy);

    /**
     * @brief Get the display name for the driver.
     * This should be overriden in the derived class and should return a common name of the device.
     * @return The device's common name.
     */
    virtual const char* getDriverName() const;
    
    /// @brief returns true if the device is valid
    operator bool() const;

    /// @brief called by the framework to initialize the device
    virtual void begin();

    inline unsigned long getFlags() const { return flags; }
    inline bool hasFlags(unsigned long f) const { return (flags & f)==f; }

    /// @brief returns the state of the sensor
    /// Derived classes should override this to return of the sensor is operational or in a degraded state.
    virtual ModuleState getState() const;
    
    /// @brief Requests the device reset.
    /// If possible the device should send physical reset events to the device. Otherwise, this will simply reset any internal device data.
    virtual void reset();

    /// @brief clear the readings
    virtual void clear();

    /// @brief Called when the device should start a new measurement
    /// In your driver or module, you can expect this method to be called every `updateInterval` period (in milliseconds). You should
    /// process your update code quickly. If you need to wait for hardware, then use the `Module::delay(millis)` method to have your 
    /// update method called again after an appropriate delay. Your `handleUpdate()` method would then have to track if entry is a fresh
    /// update or if you were waiting for a previous hardware event to complete. A simple switch() state machine usually works good.
    virtual void handleUpdate();

    /// @brief Retrieve the device alias as set by the user
    /// For example a device, such as humidity, temperature or motion sensor, can be associted with a room name by setting an alias.
    inline String getAlias() const { return alias; }

    /// @brief Return the number of slots for this device
    /// Although a device may have any number of slots up to MAX_SLOTS (typicall 256, configured in NimbleConfig.h) this count
    /// should not change once the sensor has been initialized and readings are taking place.
    inline short slotCount() const { return slots; }

    /// @brief Get the alias for the given slot (if set)
    String getSlotAlias(short slotIndex) const;

    /// Set an alias on the given slot
    void setSlotAlias(short slotIndex, String alias);

    /// find a slot number using its alias name
    short findSlotByAlias(String slotAlias) const;

    /// @brief return sensor reading for given slot index
    SensorReading& find(short index, SensorType stype);

    /// @brief return sensor reading for given slot index
    const SensorReading& find(short index, SensorType stype) const;

    /// @brief return sensor reading for given slot index
    SensorReading& find(String alias, SensorType stype);

    /// @brief return a Slot (or sub-module) by giving a slot address.
    /// Slot addresses are colon seperated slot IDs or alias. Since sub-modules are stored in slots this can
    /// also be used to return a sub-module or a slot attached to a sub-module.
    /// @param slotId string containing path to slot as a colon seperated list of slot IDs or aliases
    /// @param requiredType (optional) if supplied, require that the slot is of the expected type, or return an invalid object.
    Slot slotByAddress(const char* slotId, SensorType requiredType = AnySensorType);

    /// @brief Call a function for each slot in this module
    /// The forEach method will loop through all slots of the matching SensorType calling your callback function.
    /// Use AnySensorType to match all types. You can supply any value to pUserData (including nullptr) and the pointer
    /// value will be passed to your callback function as-is.
    /// Your callback must return true if iteration should continue, or false to immediately return from the forEach loop.
    /// You can either a real function or use a lambda expression in this form:
    /// ```
    /// forEach( [](Module::Slot& slot, void* pUserData) -> bool { slot.reading.clear(); return true; } );
    /// ```
    void forEach(SlotCallback cb, void* pUserData = nullptr, SensorType st = AnySensorType);

    /// @brief Call a function for each sensor reading in this module
    /// The forEach method will loop through all slots and thier sensor readings of the matching SensorType calling your
    ///  callback function. Use AnySensorType to match all types. You can supply any value to pUserData (including nullptr) 
    /// and the pointer value will be passed to your callback function as-is.
    /// Your callback must return true if iteration should continue, or false to immediately return from the forEach loop.
    /// You can either a real function or use a lambda expression in this form:
    /// ```
    /// forEach( [](SensorReading& reading, void* pUserData) -> bool { reading.clear(); return true; } );
    /// ```
    void forEach(ReadingCallback cb, void* pUserData = nullptr, SensorType st = AnySensorType);

    /// @brief Call a function for each sub-module in this module
    /// The forEach method will loop through all slots of the SubModule type calling your callback function. You can supply 
    /// any value to pUserData (including nullptr) and the pointer value will be passed to your callback function as-is.
    /// Your callback must return true if iteration should continue, or false to immediately return from the forEach loop.
    /// You can either a real function or use a lambda expression in this form:
    /// ```
    /// forEach( [](Module& module, void* pUserData) -> bool { module.clear(); return true; } );
    /// ```
    void forEach(ModuleCallback cb, void* pUserData = nullptr);
    
    /// @brief Schedule another update when the delay timer expires
    /// This will schedule handleUpdate() to be called after _delay milliseconds expires. It is useful for sensors that require
    /// a start measurement, then a delay, then a read from device.
    ///
    /// @param _delay The number of milliseconds to wait.
    void delay(unsigned long _delay);

    /// @brief Returns true if the most recent measurement is considered old.
    /// Stale measurements should not typical exist, but may if the sensor hardware fails to respond or is busy.
    virtual bool isStale(unsigned long _now=0) const;

    /// @brief return sensor reading for given slot index
    const SensorReading& find(String alias, SensorType stype) const;

    /// @brief return sensor reading for given slot index
    SensorReading& operator[](unsigned short slotIndex);

    /// @brief return sensor reading for given slot index
    const SensorReading& operator[](unsigned short slotIndex) const;

    /// @brief return sensor reading for given slot index
    inline SensorReading& operator[](String alias) { return find(alias, AnySensorType); }

    /// @brief return sensor reading for given slot index
    inline const SensorReading& operator[](String alias) const { return find(alias, AnySensorType); }

    SensorReading& operator[](const Rest::Argument& arg);

    /// @brief Serialize a slot reading into a Json object
    void jsonGetReading(JsonObject& node, short slot) const;

    /// @brief Serialize all slot readings for this device into a Json array
    void jsonGetReadings(JsonObject& node) const;

    /// @brief Standard RestAPI response when retrieving sensor readings for this device.
    int toJson(JsonObject& target, JsonFlags displayFlags=JsonDefault) const;

    /// @brief RestAPI methods
    /// @{
    // unfortunately we cannot bind constants in the rest handlers so we have to create these inline ones
    inline int restStatus(RestRequest& request) const { return toJson(request.response, JsonDefault); }
    inline int restSlots(RestRequest& request) const { return toJson(request.response, JsonSlots); }
    inline int restStatistics(RestRequest& request) const { return toJson(request.response, JsonStatistics); }
    inline int restDetail(RestRequest& request) const { return toJson(request.response, (JsonFlags)(JsonSlots|JsonStatistics) ); }
    /// @}

    inline bool hasEndpoints() const { return _endpoints!=nullptr; }
    inline const Endpoints* endpoints() const { return _endpoints; };
    inline Endpoints* endpoints() { return _endpoints; };

    /// @brief Return an endpoint node at the given path
    /// The endpoint will be relative to this device's endpoint, typically /api/device/[id_or_alias]. To add endpoints
    /// to the API root you will need to use the ModuleSet::on(...) method.
    inline Endpoints::Node on(const char* path) {
      if(_endpoints==nullptr) _endpoints = new Endpoints();
      return _endpoints->on(path);
    }

    /// @brief Return the root endpoint node for this device
    /// The device endpoint is typically at /api/device/[id_or_alias]. To add endpoints to the API root you will need 
    /// to use the ModuleSet::on(...) method.
    inline Endpoints::Node on() {
      if(_endpoints==nullptr) _endpoints = new Endpoints();
      return _endpoints->getRoot();
    }

  protected:
    ModuleSet* owner;
    unsigned short slots;
    Slot* readings;
    unsigned long flags;

    /// @brief Contains endpoints for this device only.
    /// By default this member will be null and is only created when a derived device requests 
    /// creation of API endpoints using the on(...) method.
    Endpoints* _endpoints;

    /// @brief how many milliseconds between device updates
    unsigned long updateInterval;

    /// @brief timestamp next update is scheduled for this device
    unsigned long nextUpdate;

    /// @brief the current device state
    /// The state describes if the device is operating normally or possibly in a degraded state due to communication, hardware or other failure
    ModuleState state;

    /// track statistics for this device
    Statistics statistics;
    
    /// @brief create a fixed number of sensor slots
    void alloc(unsigned short _slots);

    // todo: @deprecate the use of prefixUrl
    String prefixUri(const String& uri, short slot=-1) const;

    // Web interface
    // used in Display class to send raw data to client
    WebServer& http();

    // same as http's on() methods except uri is prefixed with device specific path
    void onHttp(const String &uri, WebServer::THandlerFunction handler);
    void onHttp(const String &uri, HTTPMethod method, WebServer::THandlerFunction fn);
    void onHttp(const String &uri, HTTPMethod method, WebServer::THandlerFunction fn, WebServer::THandlerFunction ufn);

    void setOwner(ModuleSet* owner);
};

extern Module NullModule;

} // ns:Nimble
