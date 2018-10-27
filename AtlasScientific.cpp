#include "AtlasScientific.h"

#define S_MEASUREMENT   1
#define S_WAITFOR_MEASUREMENT  2

namespace AtlasScientific {

  pHProbe::pHProbe(short id, SensorAddress busId, short address)
    : I2CDevice(id, busId, address, 2), measurementTime(5000)
  {
     readings[0] = SensorReading(Numeric, 0);       // probe state
     readings[1] = SensorReading(pH, VT_CLEAR, 0);  // pH value
  }

  void pHProbe::sendCommand(const char* cmd) {
    Wire.beginTransmission(address); //call the circuit by its ID number.
    Wire.write(cmd);        //transmit the command that was sent through the serial port.
    Wire.endTransmission();          //end the I2C data transmission.
  }

  ProbeResult pHProbe::readResponse()
  {
    Wire.requestFrom(address, 20, 1); //call the circuit and request 20 bytes (this may be more than we need)
    byte code = Wire.read();               //the first byte is the response code, we read this separately.

    switch (code) {                  //switch case based on what the response code is.
      case 1:                        //decimal 1.
        Serial.println("Success");   //means the command was successful.
        break;                       //exits the switch case.

      case 2:                        //decimal 2.
        Serial.println("Failed");    //means the command has failed.
        return Failed;
        break;                       //exits the switch case.

      case 254:                      //decimal 254.
        Serial.println("Pending");   //means the command has not yet been finished calculating.
        return Pending;
        break;                       //exits the switch case.

      case 255:                      //decimal 255.
        Serial.println("No Data");   //means there is no further data to send.
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

    Serial.println(ph_data);          //print the data.
    return Success;
  }

  void pHProbe::handleUpdate()
  {
    ProbeResult result;
    long& _state = readings[0].l;
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
            readings[1] = SensorReading(pH, atof(ph_data));
            _state++;
            break;
          case Failed: 
            readings[1] = InvalidReading;
            _state = 0;
            break;
          case Pending:
            break;
          case NoData:
            readings[1] = NullReading;
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
