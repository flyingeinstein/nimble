
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
        inline LogEntry() : dest(nullptr), _severity(Alert), _error(0) {}
        inline LogEntry(Logger* _dest, Severity sev, String msg) : dest(_dest), _severity(sev), _message(msg), _error(0) {}
        LogEntry(LogEntry&& empl);

        ~LogEntry();

        inline LogEntry& module(String name) { _module = name; return *this; }
        inline LogEntry& category(String cat) { _category = cat; return *this; }
        inline LogEntry& detail(String long_msg) { _detail = long_msg; return *this; }
        inline LogEntry& error(short errcode) { _error = errcode; return *this; }
        inline LogEntry& sensor(SensorAddress addr) { _sensor = addr; return *this; }

        inline Severity severity() const { return _severity; }

        void print(Stream& s);

        short toJson(JsonObject& obj);

      protected:
        Logger* dest;
        Severity _severity;
        String _message;
        String _module;
        String _category;
        String _detail;
        short _error;
        SensorAddress _sensor;
    };


    class Logger {
      public:
        Logger() : remote(true), consoleLevel(Debug), remoteLevel(Info) {}

        inline LogEntry info(String msg) { return LogEntry(this, Info, msg); }
        inline LogEntry warn(String msg) { return LogEntry(this, Warning, msg); }
        inline LogEntry debug(String msg) { return LogEntry(this, Debug, msg); }
        inline LogEntry error(String msg) { return LogEntry(this, Error, msg); }
        inline LogEntry fatal(String msg) { return LogEntry(this, Fatal, msg); }
        inline LogEntry alert(String msg) { return LogEntry(this, Alert, msg); }

        void write(LogEntry& entry);

      protected:
        bool remote;

        Severity consoleLevel;
        Severity remoteLevel;

        void write(JsonObject& msg);

        // we'll search for and resolve a logger module upon first logging request
        Endpoints::Request logEndpoint;
    };

} // ns:Nimble
