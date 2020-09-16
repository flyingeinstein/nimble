
#pragma once

#include "NimbleAPI.h"


class Influx : public Nimble::Module
{
    public:
        class Target {
            public:
                String measurement;
                int module;
                std::vector<short> slots;
                std::vector<String> aliases;

                Target();
                Target(int module, std::initializer_list<short>&& _slots, const char* measurement = nullptr);
                Target(JsonObject json);

                Module* resolve(Nimble::ModuleSet& modules) const;

                int toJson(JsonObject& target, Nimble::ModuleSet& modules) const;
        };
        
    public:
        Influx(short id);

        virtual const char* getDriverName() const;

        virtual void begin();

        virtual void handleUpdate();

        int transmit();

        /// @brief Handles Rest API requests on this module
        Rest::Endpoint delegate(Rest::Endpoint &p) override;

        int dataToJson(JsonObject& target);
        int dataFromJson(JsonObject& source);

        String format(const Target& target) const;

    protected:
        String influxUrl;
        String database;
        String measurement;
        bool enable;
        unsigned long long lastUpdate;
        String line;    // last line to go out
        std::vector<Target> targets;
};
