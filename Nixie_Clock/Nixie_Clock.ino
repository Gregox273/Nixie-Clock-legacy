#include <DS1307RTC.h>
#include <Time.h>
#include <TimeAlarms.h>
#include <Wire.h>



//Use the TimeAlarms and other time libraries
//Gregory Brooks 17/02/2015


int latchPin = 4;
int clockPin = 7;
int dataPin = A2;
boolean debug = true; //turns on serial debugging
int digits[7];

tmElements_t tm;


void setup(){
  Serial.begin(9600);
  pinMode (latchPin, OUTPUT);
  pinMode (clockPin, OUTPUT);
  pinMode (dataPin, OUTPUT);
}

void loop(){
  
  
  //assumes DS1307 has been set; for now use the settime example (DS1307 library) to set the DS1307 (then program another program so the time isn't set again!)
  if(RTC.read(tm)){

    digits[0] = tm.Hour/10;
    digits[1] = tm.Hour%10;
    digits[2] = tm.Minute/10;
    digits[3] = tm.Minute%10;
    digits[4] = tm.Second/10;
    digits[5] = tm.Second%10;
    
    if (debug){
      Serial.print(digits[0]);
      Serial.print(digits[1]);
      Serial.print(":");
      Serial.print(digits[2]);
      Serial.print(digits[3]);
      Serial.print(":");
      Serial.print(digits[4]);
      Serial.println(digits[5]);
    }
    update(digits,sizeof(digits));
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

  while (tm.Second == 10* digits[4] + digits[5]){
    //wait for second to pass, add other stuff here to do in the meantime (and remove/reduce the delay?)
    delay(100);
    RTC.read(tm);
  }
}


void update(int numbers[],int n){//array, length
  //Serial.println("Update run");
  
  //8 bit int for each chip
  int data1 = B00000000;
  int data2 = B00000000;
  int data3 = numbers[5] + (numbers[4] << 4);
  int data4 = numbers[3] + (numbers[2] << 4);
  int data5 = numbers[1] + (numbers[0] << 4);
  
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
