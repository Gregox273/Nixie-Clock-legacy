#include "Arduino.h"
#include "GPS.h"

GPS::GPS(int rxpin, int txpin):ser(rxpin, txpin)//serial port for the GPS
{
  _rxpin = rxpin;
  _txpin = txpin;
  // START OUR SERIAL DEBUG PORT
  if (_debug){
  Serial.begin(9600);
  }
}

void GPS::init(int baud){
  _baud = baud;
  if (_debug){
    Serial.println("GPS Demonstration Script");
    Serial.println("Initialising....");
  }
  // THE FOLLOWING COMMAND SWITCHES MODULE TO 4800 BAUD
  // THEN SWITCHES THE SOFTWARE SERIAL TO 4,800 BAUD
  // lower baud rate gives increased reliability over software serial
  String msg = "$PUBX,41,1,0007,0003,";
  msg += String(baud);
  msg += ",0*13\r\n";
  ser.begin(9600);
  ser.print(msg); 
  ser.begin(4800);
  ser.flush();
}

void GPS::set_stationary_mode(){
  //  THIS COMMAND SETS STATIONARY MODE AND CONFIRMS IT 
  if (_debug){
    Serial.println("Setting uBlox nav mode: ");
  }
  uint8_t setNav[]={0xB5,0x62, 0x06, 0x24, 0x24, 0x00, 0xFF, 0xFF, 0x02, 0x03, 0x00, 0x00, 0x00, 0x00, 0x10, 0x27, 0x00, 0x00, 0x05, 0x00, 0xFA, 0x00, 0xFA, 0x00, 0x64, 0x00, 0x2C, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};  
  sendUBX(setNav, sizeof(setNav)/sizeof(uint8_t));
}

void GPS::set_ubx_protocol(){
  //set output format to binary (UBX protocol)
  uint8_t setOut[]={0xB5,0x62, 0x06, 0x00, 0x14, 0x00, 0x01, 0x00, 0x00, 0x00, 0xD0, 0x08, 0x00, 0x00, 0xC0, 0x12, 0x00, 0x00, 0x03, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};  
  sendUBX(setOut, sizeof(setOut)/sizeof(uint8_t));
}

void GPS::deactivate_nav_sol(){
  //deactivate nav-sol(ecef) messages 
  uint8_t setSol[]={0xB5,0x62, 0x06, 0x01, 0x08, 0x00, 0x01, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};  
  sendUBX(setSol, sizeof(setSol)/sizeof(uint8_t));
}

void GPS::activate_sbas(){
 //activate SBAS (auto select service depending on location)
 uint8_t setSBAS[]={0xB5, 0x62, 0x06, 0x16, 0x08, 0x00, 0x01, 0x07, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
 sendUBX(setSBAS, sizeof(setSBAS)/sizeof(uint8_t));
}
    
bool GPS::gettime(){
  uint8_t setUtc[]={0xB5, 0x62, 0x01, 0x21, 0x00, 0x00, 0x22, 0x67};//poll timeutc message
  while (ser.available()){
    int temp = ser.read();
  }
  for (int x = 0; x < 8; x++){
    ser.write(setUtc[x]);
  }
  ser.flush();
  delay(700);
  if (ser.available() > 27){    
      uint8_t buf[28];//the packet from the GPS

      for (int x=0; x<28; x++){
        buf[x]=ser.read();
        //Serial.println(buf[x],HEX);
      }
      
      gpstime = {
      (uint32_t)buf[6] | (uint32_t)buf[7] << 8 |  (uint32_t)buf[8] << 16 | (uint32_t)buf[9] << 24,//iTOW
      (uint32_t)buf[10] | (uint32_t)buf[11] << 8 |  (uint32_t)buf[12] << 16 | (uint32_t)buf[13] << 24,//tAcc
      (int32_t)buf[14] | (int32_t)buf[15] << 8 |  (int32_t)buf[16] << 16 | (int32_t)buf[17] << 24,//nano
      (uint16_t)buf[18] | (uint16_t)buf[19] << 8,//Year
      (uint8_t)buf[20],//Month
      (uint8_t)buf[21],//Day
      (uint8_t)buf[22],//Hour
      (uint8_t)buf[23],//Min
      (uint8_t)buf[24],//Sec
      (uint8_t)buf[25],//Valid
      };
      if (_debug ){//&& gpstime.Year != 2015){
        Serial.print("Year: ");
        Serial.println(gpstime.Year);
        Serial.print("Month: ");
        Serial.println(gpstime.Month);
        Serial.print("Day: ");
        Serial.println(gpstime.Day);
        Serial.print("Hour: ");
        Serial.println(gpstime.Hour);
        Serial.print("Min: ");
        Serial.println(gpstime.Min);
        Serial.print("Sec: ");
        Serial.println(gpstime.Sec);
      }

      //calculate checksum
      uint8_t CK_A = 0;
      uint8_t CK_B = 0;
      for (uint8_t i=2; i<26; i++) {
        
        CK_A = CK_A + buf[i];
        CK_B = CK_B + CK_A;
      } 
      
      if (gpstime.Year == 1980 || gpstime.Valid != 7 || CK_A != buf[26] || CK_B != buf[27]){
        return false; //no fix or invalid fix
      }
      else{
        return true;
      }
    }
    return false;
}


void GPS::sendUBX(uint8_t *MSG, uint8_t len) {
  while(!_gps_set_sucess){//keep trying until GPS acknowledges message
    uint8_t CK_A = 0x00; //checksum variables
    uint8_t CK_B = 0x00;
 
    for(int I=2;I<len-2;I++)//checksum algorithm (does not use first 2 bytes)
   {
     CK_A = CK_A + MSG[I];
     CK_B = CK_B + CK_A;
   }

    MSG[len-2] = CK_A;//saves checksum to last 2 bytes of message
    MSG[len-1] = CK_B;
    for(int i=0; i<len; i++) {
      ser.write(MSG[i]);
    //Serial.print(MSG[i], HEX);//for debugging
  }
  ser.println();
    _gps_set_sucess=getUBX_ACK(MSG);
  }
_gps_set_sucess=0;//reset 'message acknowledged' variable  
}

// Calculate expected UBX ACK packet and parse UBX response from GPS
boolean GPS::getUBX_ACK(uint8_t *MSG) {
  uint8_t b;
  uint8_t ackByteID = 0;
  uint8_t ackPacket[10];
  unsigned long startTime = millis();
  //Serial.print(" * Reading ACK response: ");
 
  // Construct the expected ACK packet    
  ackPacket[0] = 0xB5;  // header
  ackPacket[1] = 0x62;  // header
  ackPacket[2] = 0x05;  // class
  ackPacket[3] = 0x01;  // id
  ackPacket[4] = 0x02;  // length
  ackPacket[5] = 0x00;
  ackPacket[6] = MSG[2];  // ACK class
  ackPacket[7] = MSG[3];  // ACK id
  ackPacket[8] = 0;   // CK_A
  ackPacket[9] = 0;   // CK_B
 
  // Calculate the checksums
  for (uint8_t i=2; i<8; i++) {
    ackPacket[8] = ackPacket[8] + ackPacket[i];
    ackPacket[9] = ackPacket[9] + ackPacket[8];
  }
 
  while (1) {
 
    // Test for success
    if (ackByteID > 9) {
      // All packets in order!
      //Serial.println(" (SUCCESS!)");
      return true;
    }
 
    // Timeout if no valid response in 3 seconds
    if (millis() - startTime > 3000) { 
      //Serial.println(" (FAILED!)");
      return false;
    }
 
    // Make sure data is available to read
    if (ser.available()) {
      b = ser.read();
 
      // Check that bytes arrive in sequence as per expected ACK packet
      if (b == ackPacket[ackByteID]) { 
        ackByteID++;
        //Serial.print(b, HEX);
      } 
      else {
        ackByteID = 0;  // Reset and look again, invalid order
      }
 
    }
  }
}

void GPS::wake(){
  if (!_awake){
    uint8_t GPSon[] = {0xB5, 0x62, 0x06, 0x04, 0x04, 0x00, 0x01, 0x00,0x09, 0x00, 0x18, 0x7A};
    for (int x = 0; x < 12; x++){
      ser.write(GPSon[x]);
    }
    ser.flush();
    _awake = true;
  }
}

void GPS::sleep(){
  if (_awake){
    uint8_t GPSoff[] = {0xB5, 0x62, 0x06, 0x04, 0x04, 0x00, 0x01, 0x00,0x08, 0x00, 0x17, 0x78};
    for (int x = 0; x < 12; x++){
      ser.write(GPSoff[x]);
    }
    ser.flush();
    _awake = false;
  }
}

