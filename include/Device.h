/**
 * @file Device.h
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
#include "Devices.h"

// Device Flags
#define DF_DISPLAY       F_BIT(0)             /// Device is some sort of display device
#define DF_BUS           F_BIT(1)             /// Device is a bus containing sub-devices
#define DF_I2C_BUS       (DF_BUS|F_BIT(2))    /// Device is an i2c bus (therefor DF_BUS flag will also be set)
#define DF_SERIAL_BUS    (DF_BUS|F_BIT(3))    /// Device is a serial bus (therefor DF_BUS flag will also be set)

class Device;
class Devices;

/**
 * @brief Base class for all devices.
 * This class provides base functionality for all devices. It provides to a base class
 * Rest and Http endpoint services and sensor/slot management.
 * 
 */
class Device {
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
    /**
     * @brief Construct a new Device object
     * 
     * @param id The device id.
     * @param _slots The number of slots for measurements this device has.
     * @param _updateInterval The number of milliseconds between successive measurements. The framework will call Device::update() each time this timer expires.
     * @param _flags Device flags; such as indicating if the device is a BUS with sub-devices.
     */
    Device(short id, short _slots, unsigned long _updateInterval=1000, unsigned long _flags=0);

    /**
     * @brief Construct a new Device object
     * 
     * @param copy Will make a copy of this device.
     */
    Device(const Device& copy);

    virtual ~Device();

    Device& operator=(const Device& copy);

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
    virtual DeviceState getState() const;
    
    /// @brief Requests the device reset.
    /// If possible the device should send physical reset events to the device. Otherwise, this will simply reset any internal device data.
    virtual void reset();

    /// @brief clear the readings
    virtual void clear();

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
    
    /// Called when the device should start a new measurement
    virtual void handleUpdate();

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
    SensorReading& operator[](unsigned short slotIndex);

    /// @brief return sensor reading for given slot index
    const SensorReading& operator[](unsigned short slotIndex) const;

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

  protected:
    Devices* owner;
    unsigned short slots;
    Slot* readings;
    unsigned long flags;
    Devices::Endpoints endpoints;


    unsigned long updateInterval;
    unsigned long nextUpdate;       // timestamp next update is scheduled for this device

    DeviceState state;
    Statistics statistics;
    
    void alloc(unsigned short _slots);

    String prefixUri(const String& uri, short slot=-1) const;
    
    // Web interface
    inline const Devices::WebServer& http() const { assert(owner); return owner->http(); }
    inline Devices::WebServer& http() { assert(owner); return owner->http(); }
    
    // Rest interface
    inline const RestRequestHandler& rest() const { assert(owner); return owner->rest(); }
    inline RestRequestHandler& rest() { assert(owner); return owner->rest(); }

    // same as http's on() methods except uri is prefixed with device specific path
    void onHttp(const String &uri, Devices::WebServer::THandlerFunction handler);
    void onHttp(const String &uri, HTTPMethod method, Devices::WebServer::THandlerFunction fn);
    void onHttp(const String &uri, HTTPMethod method, Devices::WebServer::THandlerFunction fn, Devices::WebServer::THandlerFunction ufn);

    Devices::Endpoints::Node on(const char *endpoint_expression) {
        assert(owner);
        return owner->on(endpoint_expression);   // add the rest (recursively)
    }
   
    void setOwner(Devices* owner);
    
    friend class Devices;
};

extern Device NullDevice;
