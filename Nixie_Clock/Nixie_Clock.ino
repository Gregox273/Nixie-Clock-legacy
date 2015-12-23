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
//Button pins
const int modein  = 12;
const int setin = 2;

boolean debug = false; //turns on serial debugging
uint8_t digits[7];//six digits to be displayed
uint8_t prevdigits[7];//for fading
int anodes[7] = {3,5,6,9,10,11};//nixie tube anode transistors
int bulb = 8;//neon bulb pin
unsigned long starttime = millis();
bool got_gps = false;//only get gps time once a day
bool cycled = false; //only prevent cathode poisoning once a day

//See debounce example on Arduino website:
//https://www.arduino.cc/en/Tutorial/Debounce
int mode_state = HIGH;//LOW means button pressed as they are pulled up
int prev_mode_state = HIGH;
unsigned long mode_debounce_time = 0;
bool mode_pressed = false;

int mode = 0;//0 for time, 1 for date, 2 for countdown

tmElements_t tm;//time read from RTC

GPS gps(14,13);//instance of GPS class from GPS library


void setup(){
  if (debug){
    Serial.begin(9600);
  }
  SoftwareSerial ser(14,13);//serial port for GPS (hardware serial used for debugging, communication and reprogramming)
  ser.begin(9600);
  pinMode (latchPin, OUTPUT);
  pinMode (clockPin, OUTPUT);
  pinMode (dataPin, OUTPUT);
  

  //Buttons:
  pinMode (modein, INPUT_PULLUP);
  pinMode (setin, INPUT_PULLUP);
  
  //All anode PWM pins to outputs
  for (int x = 0; x<6; x++){
    pinMode(anodes[x],OUTPUT);
    digitalWrite(anodes[x],LOW);
  }
  //neon bulbs
  pinMode(bulb,OUTPUT);
  digitalWrite(bulb,LOW);
  uint8_t placehold[7] = {8,8,8,8,8,8};
  Update(placehold, sizeof(placehold));//until other digits are set
  for (int x = 0; x<6; x++){
    digitalWrite(anodes[x],HIGH);//leave the tubes on max brightness for now
  }

  gps._debug = debug;
  gps.wake();
  gps.init(4800);
  gps.set_stationary_mode();
  gps.set_ubx_protocol();
  gps.deactivate_nav_sol();
  gps.activate_sbas();
  for (int attempt = 0; attempt < 20; attempt++){//timeout after 100 tries
    timesync();//try to get gps time on startup
  }


}




void loop(){
  unsigned long interval = 1000;//run the interval code at one second intervals
  if (modePress()){
    mode++;
    mode = mode%3;//mode 0,1,2
    
  }
  
  
  if (millis() - starttime >= interval){//if one interval (defined above) has passed
    getTime(digits,prevdigits,sizeof(digits),mode);
    
    starttime = millis();
    //Serial.println("fade");
    fade(digits,prevdigits,sizeof(digits));

    if (tm.Hour == 16){
      timesync();
      poisoning();//prevent cathode poisoning
    }
    else{
      got_gps = false;
      cycled = false;
    }
  }

  
  
}




void getTime(uint8_t numbers[],uint8_t prevnumbers[], int n, int Mode){//array, length, mode
  for (int x = 0; x < 6; x++){
    prevnumbers[x] = numbers[x];//save current digits on display to prevnumbers
  }
  //assumes DS1307 has been set; for now use the settime example (DS1307 library) to set the DS1307 (then upload another program so the time isn't set again!)
  if(RTC.read(tm)){
    if (Mode == 0){//display time
      numbers[0] = tm.Hour/10;
      numbers[1] = tm.Hour%10;
      numbers[2] = tm.Minute/10;
      numbers[3] = tm.Minute%10;
      numbers[4] = tm.Second/10;
      numbers[5] = tm.Second%10;
    }
    if (Mode == 1){//display date
      numbers[0] = tm.Day/10;
      numbers[1] = tm.Day%10;
      numbers[2] = tm.Month/10;
      numbers[3] = tm.Month%10;
      //Serial.println(tm.Year);
      uint8_t yr = (tm.Year + 1970)%100;
      numbers[4] = yr/10;
      numbers[5] = yr%10;
    }
    if (Mode == 2){//display countdown
      
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
      
      time_t systime = makeTime(tm);//current time
      time_t difftime = target - systime;; // difference between target and current time
     
      //Serial.println(difftime);
      if (difftime < 1000000){//only display if it will fit
        numbers[0] = difftime/100000;
        numbers[1] = (difftime%100000)/10000;
        numbers[2] = (difftime%10000)/1000;
        numbers[3] = (difftime%1000)/100;
        numbers[4] = (difftime%100)/10;
        numbers[5] = (difftime%10);
      }
      else{//if the number of seconds remaining won't fit
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
  digitalWrite(bulb, !digitalRead(bulb));
  for (int x = 0; x < 31; x++){
    unsigned long interval = 10*x/30;
    
    delay(interval);
    Update(prevnumbers, sizeof(prevnumbers));
    digitalWrite(bulb, !digitalRead(bulb));//fade neon bulbs
    
    delay(10-interval);
    Update(numbers, sizeof(numbers));
    digitalWrite(bulb, !digitalRead(bulb));
  }
  
}

void Update(uint8_t numbers[],int n){
  if (debug){
    Serial.println("Update run");
  }

  //8 bit int for each chip
  int data1 = B10110000;//for blue leds and no decimals;
  //colour depends on time of day
  //to do: put this in main loop
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
    data1 = B11110000;//off
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

void timesync(){
      //Serial.println(got_gps);
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

void poisoning(){
  if (!cycled){
    cycled = true;
    uint8_t numbers[7];
    for (int x = 0; x < 40; x++){
      for (int n = 0; n < 6; n++){
        numbers[n] = x%10;
      }
      //8 bit int for each chip
      int data1 = B11111111;//leds off, light decimals too

      int data2 = B11111111;//light decimals too
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
      
      delay(100);
    }
  }
}

