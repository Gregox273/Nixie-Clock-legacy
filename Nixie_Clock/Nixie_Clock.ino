#include <DS1307RTC.h>
#include <Time.h>
//#include <TimeAlarms.h>
#include <Wire.h>
//#include <GPS.h>



//Use the TimeAlarms and other time libraries

//Gregory Brooks 17/02/2015

//Shift register pins
int latchPin = 4;
int clockPin = 7;
int dataPin = A2;

boolean debug = false; //turns on serial debugging
uint8_t digits[7];//six digits to be displayed
uint8_t prevdigits[7];//for fading
int anodes[7] = {3,5,6,9,10,11};//nixie tube anode transistors
int bulb = 8;//neon bulb pin
unsigned long starttime = millis();

tmElements_t tm;


void setup(){
  Serial.begin(9600);
  pinMode (latchPin, OUTPUT);
  pinMode (clockPin, OUTPUT);
  pinMode (dataPin, OUTPUT);
  //All anode PWM pins to outputs
  
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
}




void loop(){
  unsigned long interval = 1000;
  if (millis() - starttime >= interval){//if one second has passed
    getTime(digits,prevdigits,sizeof(digits));
    
    

    starttime = millis();
    fade(digits,prevdigits,sizeof(digits));
  }

  
}




void getTime(uint8_t numbers[],uint8_t prevnumbers[], int n){//array, length
  for (int x = 0; x < 6; x++){
    prevnumbers[x] = numbers[x];
  }
  //assumes DS1307 has been set; for now use the settime example (DS1307 library) to set the DS1307 (then upload another program so the time isn't set again!)
  if(RTC.read(tm)){

    numbers[0] = tm.Hour/10;
    numbers[1] = tm.Hour%10;
    numbers[2] = tm.Minute/10;
    numbers[3] = tm.Minute%10;
    numbers[4] = tm.Second/10;
    numbers[5] = tm.Second%10;
    
    if (debug){
      Serial.print(numbers[0]);
      Serial.print(numbers[1]);
      Serial.print(":");
      Serial.print(numbers[2]);
      Serial.print(numbers[3]);
      Serial.print(":");
      Serial.print(numbers[4]);
      Serial.println(numbers[5]);
    }
    
  }
  else {
    if (RTC.chipPresent() && debug == true) {
      Serial.println("The DS1307 is stopped.  Please run the SetTime");
      Serial.println("example to initialize the time and begin running.");
      Serial.println();
    }
    else {
      if (debug==true){
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
    data1 = B1011000;//blue
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
