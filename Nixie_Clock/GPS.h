/*
GPS (uBlox neo 6m) time sync for nixie clock
Gregory Brooks 10/12/2015
Released into public domain
*/
#ifndef GPS_h
#define GPS_h
#include "Arduino.h"
#include <SoftwareSerial.h>
class GPS
{
  public:
    GPS(int rxpin, int txpin);
    void init(int baud);
    void set_stationary_mode();
    void set_ubx_protocol();
    void deactivate_nav_sol();
    void activate_sbas();
    bool gettime(); //false if no time fix
    
    void sendUBX(uint8_t *MSG, uint8_t len);
    boolean getUBX_ACK(uint8_t *MSG);
    void wake();
    void sleep();
    
    
    struct GPStime{    //the received nav-utc packet
       uint32_t iTOW;
       uint32_t tAcc;
       int32_t nano;
       uint16_t Year;
       uint8_t Month;
       uint8_t Day;
       uint8_t Hour;
       uint8_t Min;
       uint8_t Sec;
       uint8_t Valid;
      };

      GPStime gpstime;
      bool _debug = false;
      bool _awake = false;
      
  private:
    byte _gps_set_sucess = 0 ;//global variable
    int _rxpin;
    int _txpin;
    int _baud;
    
    SoftwareSerial ser;
};
#endif
