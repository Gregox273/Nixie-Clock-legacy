//Gregory Brooks 16/02/2015
//For testing nixie display hardware
int latchPin = 4;
int clockPin = 7;
int dataPin = A2;
int anodes[7] = {3,5,6,9,10,11};
int bulb = 8;//neon bulb pin
uint8_t data1 = B01001111;
int packet[] = {0,0,0,0,0,0};//6 digits

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
  digitalWrite(bulb,HIGH);
  
  int placehold[7] = {8,8,8,8,8,8};
  update(placehold, sizeof(placehold));
  for (int x = 0; x<6; x++){
    digitalWrite(anodes[x],HIGH);//leave the tubes on max brightness for v0.1
    //light the bulbs
    digitalWrite(8, HIGH);
  }
}

void loop(){
  
  /*if (Serial.available()>=6){
    Serial.println("6 digits received");
    for (int x = 0; x<6; x++){
      packet[x] = (int(Serial.read()) - 48);
      Serial.println (packet[x]);
    }
    update(packet,sizeof(packet));
  }  
  else{  
   //clear the buffer
   while (Serial.available()){
      int x = Serial.read();
   }
  }
  */
  if (digitalRead(8)){
    digitalWrite(8,LOW);
  }
  else{
    digitalWrite(8,HIGH);
  }
  update(packet,sizeof(packet));
  delay(1000);
  for (int x = 0; x < 6; x++){
    if (packet[x] == 9){
      packet[x] = 0;
    }
    else{
      packet[x] = packet[x]+1;
    }
  }
}
  
     
  
    
 
  
  


void update(int numbers[],int n){//array, length
  Serial.println("Update run");
  
  
  //8 bit int for each chip
  /*
  if (data1 == 128){
    data1 = 1;
  }
  else{
    data1 = data1*2;
  }
  */
  
  
  int data2 = numbers[4] + (numbers[5] << 4);
  int data3 = numbers[2] + (numbers[3] << 4);
  int data4 = numbers[0] + (numbers[1] << 4);
  
  //send values to shift registers
  digitalWrite(latchPin,LOW);
  shiftOut(dataPin, clockPin, MSBFIRST,255 - data1);
  shiftOut(dataPin, clockPin, MSBFIRST, B00000000);
  shiftOut(dataPin, clockPin, MSBFIRST,data2);
  shiftOut(dataPin, clockPin, MSBFIRST,data3);
  shiftOut(dataPin, clockPin, MSBFIRST,data4);
  digitalWrite(latchPin, HIGH);
  Serial.println (255 - data1,BIN);//debug
  Serial.println (data2,BIN);//debug
  Serial.println (data3,BIN);//debug
  Serial.println (data4,BIN);//debug
}
