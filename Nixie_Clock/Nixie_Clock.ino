#include <DS1307RTC.h>
#include <Time.h>
#include <SoftwareSerial.h>
//#include <TimeAlarms.h>
#include <Wire.h>
#include "GPS.h"



//Use the TimeAlarms and other time libraries

//Gregory Brooks 17/02/2015

//Shift register pins
const int latchPin = 4;
const int clockPin = 7;
const int dataPin = A2;
const int modein  = 12;
const int setin = 2;

boolean debug = false; //turns on serial debugging
uint8_t digits[7];//six digits to be displayed
uint8_t prevdigits[7];//for fading
int anodes[7] = {3,5,6,9,10,11};//nixie tube anode transistors
int bulb = 8;//neon bulb pin
unsigned long starttime = millis();
bool got_gps = false;//only get gps time once a day

int mode_state = HIGH;
int prev_mode_state = HIGH;
unsigned long mode_debounce_time = 0;
bool mode_pressed = false;

int mode = 0;//0 for time, 1 for date, 2 for countdown

tmElements_t tm;

GPS gps(14,13);



void setup(){
  //if (debug){
    Serial.begin(9600);
  //}
  SoftwareSerial ser(14,13);
  ser.begin(9600);
  pinMode (latchPin, OUTPUT);
  pinMode (clockPin, OUTPUT);
  pinMode (dataPin, OUTPUT);
  //All anode PWM pins to outputs

  //Buttons:
  pinMode (modein, INPUT_PULLUP);
  pinMode (setin, INPUT_PULLUP);
  
  for (int x = 0; x<6; x++){
    pinMode(anodes[x],OUTPUT);
    digitalWrite(anodes[x],LOW);
  }
  //neon bulbs
  pinMode(bulb,OUTPUT);
  digitalWrite(bulb,LOW);
  uint8_t placehold[7] = {8,8,8,8,8,8};
  Update(placehold, sizeof(placehold));
  for (int x = 0; x<6; x++){
    digitalWrite(anodes[x],HIGH);//leave the tubes on max brightness for v0.1
  }

  gps._debug = debug;
  gps.init(4800);
  gps.set_stationary_mode();
  gps.set_ubx_protocol();
  gps.deactivate_nav_sol();
  gps.activate_sbas();
  bool override = true;
  for (int attempt = 0; attempt < 100; attempt++){//timeout after 100 tries
    timesync(override);
  }


}




void loop(){
  unsigned long interval = 1000;
  if (modePress()){
    mode++;
    mode = mode%3;//mode 0,1,2
    
  }
  
  if (millis() - starttime >= interval){//if one second has passed
    getTime(digits,prevdigits,sizeof(digits),mode);
    
    starttime = millis();
    fade(digits,prevdigits,sizeof(digits));

    timesync(false);
    

    
    
  }
  
  
}




void getTime(uint8_t numbers[],uint8_t prevnumbers[], int n, int Mode){//array, length, mode
  for (int x = 0; x < 6; x++){
    prevnumbers[x] = numbers[x];
  }
  //assumes DS1307 has been set; for now use the settime example (DS1307 library) to set the DS1307 (then upload another program so the time isn't set again!)
  if(RTC.read(tm)){
    if (Mode == 0){
      numbers[0] = tm.Hour/10;
      numbers[1] = tm.Hour%10;
      numbers[2] = tm.Minute/10;
      numbers[3] = tm.Minute%10;
      numbers[4] = tm.Second/10;
      numbers[5] = tm.Second%10;
    }
    if (Mode == 1){
      numbers[0] = tm.Day/10;
      numbers[1] = tm.Day%10;
      numbers[2] = tm.Month/10;
      numbers[3] = tm.Month%10;
      //Serial.println(tm.Year);
      uint8_t yr = (tm.Year + 1970)%100;
      numbers[4] = yr/10;
      numbers[5] = yr%10;
    }
    if (Mode == 2){
      
      //See http://macetech.com/blog/node/115 for this time difference method:
      tmElements_t target_elements;
      target_elements.Second = 0;
      target_elements.Minute = 0;
      target_elements.Hour = 0;
      target_elements.Wday = 6;//Friday
      target_elements.Day = 25;
      target_elements.Month = 12;
      target_elements.Year = (2015 - 1970);
      time_t target = makeTime(target_elements);
      
      time_t systime = makeTime(tm);
      time_t difftime = target - systime;; // difference between current and target time
     
      //Serial.println(difftime);
      if (difftime < 1000000){
        numbers[0] = difftime/100000;
        numbers[1] = (difftime%100000)/10000;
        numbers[2] = (difftime%10000)/1000;
        numbers[3] = (difftime%1000)/100;
        numbers[4] = (difftime%100)/10;
        numbers[5] = (difftime%10);
      }
      else{
        for (int x = 0; x < 6; x++){
          numbers[x] = 9;
        }
      }
    }
    if (debug){
      Serial.print(numbers[0]);
      Serial.print(numbers[1]);
      Serial.print(":");
      Serial.print(numbers[2]);
      Serial.print(numbers[3]);
      Serial.print(":");
      Serial.print(numbers[4]);
      Serial.println(numbers[5]);
      //Serial.println(tm.Hour);
    }
    
  }
  else {
    if (RTC.chipPresent() && debug) {
      Serial.println("The DS1307 is stopped.  Please run the SetTime");
      Serial.println("example to initialize the time and begin running.");
      Serial.println();
    }
    else {
      if (debug){
        Serial.println("DS1307 read error!  Please check the circuitry.");
        Serial.println();
      }
    }
    delay(9000);
  }
}

void fade(uint8_t numbers[],uint8_t prevnumbers[],int n){
  digitalWrite(bulb, !digitalRead(bulb));//flash neon bulbs once a second
  for (int x = 0; x < 31; x++){
    unsigned long interval = 10*x/30;
    
    delay(interval);
    Update(prevnumbers, sizeof(prevnumbers));
    digitalWrite(bulb, !digitalRead(bulb));//flash neon bulbs once a second
    
    delay(10-interval);
    Update(numbers, sizeof(numbers));
    digitalWrite(bulb, !digitalRead(bulb));//flash neon bulbs once a second
  }
  
}

void Update(uint8_t numbers[],int n){
  if (debug){
    Serial.println("Update run");
  }

  //8 bit int for each chip
  int data1 = B10110000;//for blue leds and no decimals;//colour depends on time of day
  if (16<=tm.Hour && tm.Hour<22){
    data1 = B11010000;//red
  }
  else if (8<=tm.Hour && tm.Hour<12){
    data1 = B10110000;//blue
  }
  else if (12<=tm.Hour && tm.Hour<16){
    data1 = B01110000;//green
  }
  else{
    data1 = B1111000;//off
  }
  
  
  int data2 = B00000000;//no decimal points
  int data3 = numbers[4] + (numbers[5] << 4);
  int data4 = numbers[2] + (numbers[3] << 4);
  int data5 = numbers[0] + (numbers[1] << 4);
  
  //send values to shift registers
  digitalWrite(latchPin,LOW);
  shiftOut(dataPin, clockPin, MSBFIRST,data1);
  shiftOut(dataPin, clockPin, MSBFIRST,data2);
  shiftOut(dataPin, clockPin, MSBFIRST,data3);
  shiftOut(dataPin, clockPin, MSBFIRST,data4);
  shiftOut(dataPin, clockPin, MSBFIRST,data5);
  digitalWrite(latchPin, HIGH);

  if (debug){
    Serial.println (data1);//debug
    Serial.println (data2);//debug
    Serial.println (data3);//debug
    Serial.println (data4);//debug
    Serial.println (data5);//debug
  }
  
}

void timesync(bool override){
  //Serial.println(override);
  if (tm.Hour == 15|| override){
    
      if (got_gps == false){
        gps.wake();
        
        got_gps = gps.gettime();
        
        if (got_gps == true){
          tm.Year = (gps.gpstime.Year - 1970);
          tm.Month = gps.gpstime.Month;
          tm.Day = gps.gpstime.Day;
          
          tm.Hour = gps.gpstime.Hour;
          tm.Minute = gps.gpstime.Min;
          tm.Second = gps.gpstime.Sec;
          RTC.write(tm);
          
            //Serial.println("Written to RTC");
          gps.sleep();
        }
      }
    }
    else{
      got_gps = false;
    }
    //Serial.println(gps._awake);
    //Serial.println(got_gps);
    //Serial.println("");
}

bool modePress(){
  int buttonpress = 70;//button press duration for debounce
  mode_state = digitalRead(modein);
  if (mode_state != prev_mode_state){
    mode_debounce_time = millis();
  }
  if (millis() - mode_debounce_time > buttonpress){
    if (mode_state == LOW && !mode_pressed){
      mode_pressed = true;
      return true;
    }
    else if (mode_state == HIGH){
      mode_pressed = false;
    }
  }
  prev_mode_state = mode_state;
  return false;
}

