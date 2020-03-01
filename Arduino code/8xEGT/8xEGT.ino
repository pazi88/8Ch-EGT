#include "MAX31855.h"
#ifdef ARDUINO_BLUEPILL_F103C8
HardwareSerial Serial3(USART3); //for some reason this isn't defined in arduino_core_stm32
#endif

#define Sensor1_4_CAN_ADDRESS   0x20A //the data from sensers 1-4 will be sent using this address
#define Sensor5_8_CAN_ADDRESS   0x20B //the data from sensers 5-8 will be sent using this address

uint8_t cs[8] = { PA0, PA1, PA2, PA3, PB0, PB1, PB13, PB12 }; //chip select pins for the MAX31855 chips
int32_t rawData[8]  = { 0 }; //raw data from all 8 MAX31855 chips
float EGT[8]  = { 0 }; //calculated EGT values from all 8 MAX31855 chips
float ColdJunction[8]  = { 0 }; //calculated cold junction temperatures from all 8 MAX31855 chips, not needed for any other than debugging
uint8_t data14[8];    // data from EGT 1-4 that will be sent to CAN bus
uint8_t data58[8];    // data from EGT 5-8 that will be sent to CAN bus
uint16_t CanAddress;
byte canin_channel;
byte MAX31855_OK_bits; // this is used to check if all MAX31855 chips are working or even installed and code skips the ones that don't work.

MAX31855 MAX31855_chips[8] = {
  MAX31855(cs[0]),
  MAX31855(cs[1]),
  MAX31855(cs[2]),
  MAX31855(cs[3]),
  MAX31855(cs[4]),
  MAX31855(cs[5]),
  MAX31855(cs[6]),
  MAX31855(cs[7])
};

void setup() {
  MAX31855_OK_bits = 0;
  Serial.begin(115200); //debug
  Serial3.begin(115200); //data to speeduino

// begin communication to MAX31855 chips
  for (int i=0; i<8; i++) {
    MAX31855_chips[i].begin();
  }

// check that all MAX31855 chips work or are even installed
  for (int i=0; i<8; i++) {
    if (MAX31855_chips[i].readRawData() != 0){  //we will just get 0 if there is no chip at all.
      if (MAX31855_chips[i].getChipID() != MAX31855_ID) //if there is something there, check that it responds like MAX31855 (maybe useless step)
      {
        Serial.print("MAX31855 ");
        Serial.print(i+1);
        Serial.println(" Error");
      }
      else{
        Serial.print("MAX31855 ");
        Serial.print(i+1);
        Serial.println(" Ok");
        bitSet(MAX31855_OK_bits, i); //set corresponding bit to 1
      }
    }
    else{
      Serial.print("Can't find MAX31855 "); //nothing in this CS line
      Serial.println(i+1);
    }
  }
}

void CheckDataRequest(){
  if (Serial3.read() == 'R'){
     byte tmp0;
     byte tmp1;
     if ( Serial3.available() >= 3)
     {
      canin_channel = Serial3.read();
      tmp0 = Serial3.read();  //read in lsb of source can address 
      tmp1 = Serial3.read();  //read in msb of source can address
      CanAddress = tmp1<<8 | tmp0 ;
      if (CanAddress == Sensor1_4_CAN_ADDRESS || CanAddress == Sensor5_8_CAN_ADDRESS){  //check if the speeduino request includes adresses for EGTs
        SendDataToSpeeduino();  //request ok, send the data to speeduino
      }
     }
  }
}

void SendDataToSpeeduino(){
  Serial3.write("G");                      // reply "G" cmd
  Serial3.write(1);                        //send 1 to confirm cmd received and valid
  Serial3.write(canin_channel);            //confirms the destination channel
  if (CanAddress == Sensor1_4_CAN_ADDRESS){
    for (int i=0; i<8; i++) {
        Serial3.write(data14[i]);
    }
  }
  if (CanAddress == Sensor5_8_CAN_ADDRESS){
    for (int i=0; i<8; i++) {
        Serial3.write(data58[i]);
    }
  }
}

void loop() {
   for (int i=0; i<8; i++) {
    if (bitRead(MAX31855_OK_bits, i) == 1){
     rawData[i] = MAX31855_chips[i].readRawData();
     EGT[i]  = (MAX31855_chips[i].getTemperature(rawData[i]));
     ColdJunction[i]  = (MAX31855_chips[i].getColdJunctionTemperature(rawData[i]));
     if (i < 4){
      data14[2*i] = lowByte(uint16_t(EGT[i]));
      data14[2*i+1] = highByte(uint16_t(EGT[i]));
     }
     else{
      data58[2*i-8] = lowByte(uint16_t(EGT[i]));
      data58[2*i-7] = highByte(uint16_t(EGT[i]));
     }
      if (Serial3.available () > 0) {  //is there data on serial3, presumably from speeduino
        CheckDataRequest(); //there is data, but is it request from speeduino and is it for EGTs
      }
      else{ //no data request from speeduino, so broadcast to CAN bus
        //      CanSend(); // not implemented yet
      }
     Serial.print("EGT");
     Serial.print(i+1);
     Serial.print(" Temp: ");
     Serial.println(EGT[i]);
     Serial.print("Cold Junction");
     Serial.print(i+1);
     Serial.print(" Temp: ");
     Serial.println(ColdJunction[i]);
    }
  }
}
