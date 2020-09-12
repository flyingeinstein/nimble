
#pragma once

#include "NimbleAPI.h"

#include <WiFiUdp.h>


class GelfLogger : public Nimble::Module
{
    public:
        GelfLogger(short id);

        virtual const char* getDriverName() const;

        virtual void begin();

        virtual void handleUpdate();

        void log(JsonObject& node);

        /// @brief Handles Rest API requests on this module
        Rest::Endpoint delegate(Rest::Endpoint &p) override;

    protected:
        WiFiUDP Udp;

        IPAddress destip;
        String host;
        unsigned short port;
};
