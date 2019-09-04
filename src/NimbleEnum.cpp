//
// Created by guru on 9/4/19.
//

#include "NimbleEnum.h"

const char* SensorTypeName(SensorType st)
{
    switch(st) {
        case Invalid: return "invalid";
//        case ChildModule: return "module";      /// @todo not sure I like SensorType::ChildModule (slots should be simpler and not contain other modules)
        case Numeric: return "numeric";
        case Timestamp: return "timestamp";
        case Milliseconds: return "milliseconds";
        case Humidity: return "humidity";
        case Hygrometer: return "hygrometer";
        case Temperature: return "temperature";
        case HeatIndex: return "heatindex";
        case Illuminance: return "illuminance";
        case pH: return "pH";
        case ORP: return "ORP";
        case DissolvedOxygen: return "dissolved oxygen";
        case Conductivity: return "conductivity";
        case CO2: return "CO2";
        case Pressure: return "pressure";
        case Flow: return "flow";
        case Altitude: return "altitude";
        case AirPressure: return "air pressure";
        case AirQuality: return "air quality";
        case Voltage: return "voltage";
        case Current: return "current";
        case Watts: return "watts";
        case Motion: return "motion";
        default: return "n/a";
    }
}

const char* ModuleStateName(ModuleState st)
{
    switch(st) {
        case Offline: return "offline";
        case Degraded: return "degraded";
        case Nominal: return "nominal";
        default:
            return "unknown";
    }
}
