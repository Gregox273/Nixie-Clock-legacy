//Gregory Brooks 16/02/2015
//For testing nixie display hardware
int latchPin = 4;
int clockPin = 7;
int dataPin = A2;


void setup(){
  Serial.begin(9600);
  pinMode (latchPin, OUTPUT);
  pinMode (clockPin, OUTPUT);
  pinMode (dataPin, OUTPUT);
}

void loop(){
  int packet[7] = {0,0,0,0,0,0};//6 digits
  if (Serial.available()>=6){
    Serial.println("6 digits received");
    for (int x = 0; x<6; x++){
      packet[x] = (int(Serial.read()) - 48);
      Serial.println (packet[x]);
    }
    update(packet,7);
  }  
  else{  
   //clear the buffer
   while (Serial.available()){
      int x = Serial.read();
   }
  }
  delay(1000);
}
  
     
  
    
 
  
  


void update(int numbers[],int n){//array, length
  Serial.println("Update run");
  
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
  Serial.println (data1);//debug
  Serial.println (data2);//debug
  Serial.println (data3);//debug
  Serial.println (data4);//debug
  Serial.println (data5);//debug
}
