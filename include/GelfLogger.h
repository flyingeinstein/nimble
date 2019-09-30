
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

    protected:
        WiFiUDP Udp;
        IPAddress dest;
};
