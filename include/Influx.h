
#pragma once

#include "NimbleAPI.h"


class Influx : public Nimble::Module
{
    public:
        Influx(short id);

        virtual const char* getDriverName() const;

        virtual void begin();

        virtual void handleUpdate();

        int transmit();

        /// @brief Handles Rest API requests on this module
        Rest::Endpoint delegate(Rest::Endpoint &p) override;

    protected:
        String influxUrl;
        String database;
        String measurement;
        String data;
        String line;    // last line to go out
        // timestamp precision?
};
