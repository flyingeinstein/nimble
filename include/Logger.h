
#pragma once

#include "NimbleConfig.h"
#include "SensorReading.h"


namespace Nimble {

    typedef enum {
        Debug = 7,
        Info = 6,
        Warning = 5,
        Error = 4,
        Fatal = 3,
        Alert = 1     // sensor alert
    } Severity;

    class Logger;

    class LogEntry
    {
      public:
        inline LogEntry() : dest(nullptr), _severity(Alert) {}
        inline LogEntry(Logger* _dest, Severity sev) : dest(_dest), _severity(sev) {}
        LogEntry(LogEntry&& empl);

        ~LogEntry();

        inline LogEntry& module(String name) { _module = name; return *this; }
        inline LogEntry& category(String cat) { _category = cat; return *this; }
        inline LogEntry& message(String short_msg) { _message = short_msg; return *this; }
        inline LogEntry& detail(String long_msg) { _detail = long_msg; return *this; }
        inline LogEntry& error(short errcode) { _error = errcode; return *this; }
        inline LogEntry& sensor(SensorAddress addr) { _sensor = addr; return *this; }

        short toJson(JsonObject& obj);

      protected:
        Logger* dest;
        String _module;
        String _category;
        String _message;
        String _detail;
        short _error;
        Severity _severity;
        SensorAddress _sensor;
    };


    class Logger {
      public:

        inline LogEntry info() { return LogEntry(this, Info); }
        inline LogEntry warn() { return LogEntry(this, Warning); }
        inline LogEntry debug() { return LogEntry(this, Debug); }
        inline LogEntry error() { return LogEntry(this, Error); }
        inline LogEntry fatal() { return LogEntry(this, Fatal); }
        inline LogEntry alert() { return LogEntry(this, Alert); }

        void write(LogEntry* entry);

      protected:
        void write(JsonObject& msg);

        // we'll search for and resolve a logger module upon first logging request
        Endpoints::Request logEndpoint;
    };

} // ns:Nimble
