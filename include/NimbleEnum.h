//
// Created by guru on 9/4/19.
//

#pragma once


typedef enum SensorType {
    Invalid,
    ChildModule,   // slot is a child-device (such as a device on a bus)
    Numeric,       // general numeric value
    Timestamp,     // a unix timestamp
    Milliseconds,  // a measurement in milliseconds
    Humidity,      // Relative Humidity (RH)
    Hygrometer,
    Temperature,   // Celcius or Farenheit
    HeatIndex,     // Celcius or Farenheit (adjusted with RH)
    Illuminance,   // Lux
    pH,
    ORP,           // unitless
    DissolvedOxygen, // mg/L
    Conductivity,  // microS/cm (Atlas Scientific)
    CO2,           // ppm
    Pressure,      // KPa?
    Flow,          // liquid or gas flow
    Altitude,      // meters or feet
    AirPressure,
    AirQuality,
    Voltage,
    Current,
    Watts,
    Motion,
    FirstSensorType=Humidity,   // types before this are considered generic such as configuration values
    LastSensorType=Motion
} SensorType;

const char* SensorTypeName(SensorType st);


typedef enum  {
    Offline,
    Degraded,
    Nominal
} ModuleState;

const char* ModuleStateName(ModuleState st);

