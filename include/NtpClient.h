
#pragma once

#include <NTPClient.h>

#include "NimbleAPI.h"


class NtpClient : public Nimble::Module
{
    public:
        NtpClient(short id);

        virtual const char* getDriverName() const;

        inline const NTPClient& ntp() const { return _ntp; }
        inline NTPClient& ntp() { return _ntp; }

        virtual void begin();

        virtual void handleUpdate();

        void log(JsonObject& node);

        /// @brief Handles Rest API requests on this module
        Rest::Endpoint delegate(Rest::Endpoint &p) override;

    protected:
        String influxUrl;
        String database;
        String measurement;
        // timestamp precision?

        WiFiUDP _udp;
        NTPClient _ntp;
};
