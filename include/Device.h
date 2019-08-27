
#pragma once

#include "NimbleConfig.h"
#include "SensorReading.h"
#include "DeviceManager.h"

// Device Flags
#define DF_DISPLAY       F_BIT(0)
#define DF_BUS           F_BIT(1)
#define DF_I2C_BUS       (DF_BUS|F_BIT(2))
#define DF_SERIAL_BUS    (DF_BUS|F_BIT(3))

class Device;
class Devices;


class Device {
  public:
    short id;
    String alias;

  public:
    // flags controlling toJson output detail
    typedef enum : int {
      JsonSlots        = 1,
      JsonStatistics   = 2,
      JsonDefault      = JsonSlots
    } JsonFlags;
  
    class Slot {
      public:
        String alias;
        SensorReading reading;
    };

    class Statistics {
      public:
        int updates;
        
        struct {
          int bus;      // errors communicating with the sensor
          int sensing;    // errors reported by sensor (failure to sense)
        } errors;

        inline Statistics() : updates(0) { memset(&errors, 0, sizeof(errors)); }
        
        void toJson(JsonObject& target) const;
    };

  public:
    Device(short id, short _slots, unsigned long _updateInterval=1000, unsigned long _flags=0);
    Device(const Device& copy);
    virtual ~Device();
    Device& operator=(const Device& copy);

    virtual const char* getDriverName() const;
    
    operator bool() const;

    virtual void begin();

    inline unsigned long getFlags() const { return flags; }
    inline bool hasFlags(unsigned long f) const { return (flags & f)==f; }

    virtual DeviceState getState() const;
    
    // reset the device
    virtual void reset();

    // clear the readings
    virtual void clear();

    inline String getAlias() const { return alias; }

    // return the number of slots
    inline short slotCount() const { return slots; }

    String getSlotAlias(short slotIndex) const;
    void setSlotAlias(short slotIndex, String alias);

    short findSlotByAlias(String slotAlias) const;
    
    virtual void handleUpdate();

    void delay(unsigned long _delay);

    virtual bool isStale(unsigned long _now=0) const;

    // read readings
    SensorReading& operator[](unsigned short slotIndex);
    const SensorReading& operator[](unsigned short slotIndex) const;

    // json interface
    void jsonGetReading(JsonObject& node, short slot) const;
    void jsonGetReadings(JsonObject& node) const;

    // standard rest methods
    int toJson(JsonObject& target, JsonFlags displayFlags=JsonDefault) const;

    // rest methods
    // unfortunately we cannot bind constants in the rest handlers so we have to create these inline ones
    inline int restStatus(RestRequest& request) const { return toJson(request.response, JsonDefault); }
    inline int restSlots(RestRequest& request) const { return toJson(request.response, JsonSlots); }
    inline int restStatistics(RestRequest& request) const { return toJson(request.response, JsonStatistics); }
    inline int restDetail(RestRequest& request) const { return toJson(request.response, (JsonFlags)(JsonSlots|JsonStatistics) ); }
    

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
   
    // enable direct access to slots via http or mqqt
    // deprecated: void enableDirect(short slot, bool _get=true, bool _post=true);

    // http handlers
    // deprecated: void httpGetReading(short slot);
    // deprecated: void httpPostValue(short slot);

    void setOwner(Devices* owner);
    
    friend class Devices;
};

extern Device NullDevice;
