#include "AtlasScientific.h"

#define S_MEASUREMENT   1
#define S_WAITFOR_MEASUREMENT  2


//#define DEBUG_PRINT(x)  Serial.println(x)
#define DEBUG_PRINT(x)

using namespace Nimble;


namespace AtlasScientific {

  EzoProbe::EzoProbe(short id, Nimble::SensorType stype, short _address)
    : Nimble::I2C::Device(id, _address, 2), measurementTime(5000), sensorType(stype)
  {
    switch(stype) {
      case pH: address = 99; break;
      case ORP: address = 98; break;
      case DissolvedOxygen: address = 97; break;
      case Conductivity: address = 100; break;
      case CO2: address = 105; break;
      case Temperature: address = 102; break;
      // case Flow:   // apparently Atlas Scientific flow meter doesnt support i2c yet although there is an updated EZO
      default:
        address = 0;
    }
     (*this)[0] = SensorReading(Numeric, 0);       // probe state
     (*this)[1] = SensorReading(stype, VT_CLEAR, 0);  // pH value
  }

  const char* EzoProbe::getDriverName() const
  {
    return "AtlasScientific-EZO";
  }

  void EzoProbe::sendCommand(const char* cmd) {
    Wire.beginTransmission(address); //call the circuit by its ID number.
    Wire.write(cmd);        //transmit the command that was sent through the serial port.
    Wire.endTransmission();          //end the I2C data transmission.
  }

  EzoProbeResult EzoProbe::readResponse()
  {
    Wire.requestFrom(address, 20, 1); //call the circuit and request 20 bytes (this may be more than we need)
    byte code = Wire.read();               //the first byte is the response code, we read this separately.

    switch (code) {                  //switch case based on what the response code is.
      case 1:                        //decimal 1.
        DEBUG_PRINT("Success");   //means the command was successful.
        break;                       //exits the switch case.

      case 2:                        //decimal 2.
        DEBUG_PRINT("Failed");    //means the command has failed.
        return Failed;
        break;                       //exits the switch case.

      case 254:                      //decimal 254.
        DEBUG_PRINT("Pending");   //means the command has not yet been finished calculating.
        return Pending;
        break;                       //exits the switch case.

      case 255:                      //decimal 255.
        DEBUG_PRINT("No Data");   //means there is no further data to send.
        return NoData;
        break;                       //exits the switch case.
    }

   short i=0;
   byte in_char;
   while (Wire.available()) {         //are there bytes to receive.
      in_char = Wire.read();           //receive a byte.
      ph_data[i] = in_char;            //load this byte into our array.
      i += 1;                          //incur the counter for the array element.
      if (in_char == 0) {              //if we see that we have been sent a null command.
        i = 0;                         //reset the counter i to 0.
        Wire.endTransmission();        //end the I2C data transmission.
        break;                         //exit the while loop.
      }
    }

    return Success;
  }

  void EzoProbe::handleUpdate()
  {
    EzoProbeResult result;
    long& _state = (*this)[0].l;
    switch(_state) {
      case 0:
        // initialize then proceed to calibrate or measure
        _state++;
        break;
         
      case S_MEASUREMENT:
        // trigger a measurement
        sendCommand("R");
        delay(measurementTime);
        _state++;
        break;

      case S_WAITFOR_MEASUREMENT:
        // if we are done measurement, then reset state
        result = readResponse();
        switch(result) {
          case Success: 
            (*this)[1] = SensorReading(sensorType, atof(ph_data));
            _state++;
            break;
          case Failed: 
            (*this)[1] = InvalidReading;
            _state = 0;
            break;
          case Pending:
            break;
          case NoData:
            (*this)[1] = NullReading;
            _state = 0;
            break;
        }
        break;

        // other states
        // * Slope - return Slope of probe (compares probe to ideal probe)
        // * Status - return status of probe (returns 
        // * T - Temperature compensation
        // * Cal - perform calibration (will have to be a command from UI)
        // * Export/Import - calibration data
        // * i - device info (returns firmware, we could put this in a reading)

       default:
         _state = 0;
         break;
     }

     state = Nominal;
  }

}
