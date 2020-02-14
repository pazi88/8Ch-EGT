#include "MAX31855.h"

#define EGT_1-4_CAN_ADDRESS   0x201
#define EGT_5-8_CAN_ADDRESS   0x202

uint8_t cs[8] = { PA0, PA1, PA2, PA3, PB0, PB1, PB13, PB12 };
int32_t rawData[8]  = { 0 };
float EGT[8]  = { 0 };
float ColdJunction[8]  = { 0 };
uint8_t data14[8];    // data from EGT 1-4 that will be sent to CAN bus
uint8_t data58[8];    // data from EGT 1-4 that will be sent to CAN bus

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
uint8_t currentSerialCommand;

void setup() {
  byte NumberOfEGTs = 0;
  byte NumberOfChannels = 0;
  Serial.begin(115200); //debug
  Serial3.begin(115200); //data to speeduino

// begin communication to MAX31855 chips
  for (int i=0; i<8; i++) {
    MAX31855_chips[i].begin();
  }

// check that all work
  for (int i=0; i<8; i++) {
    while (MAX31855_chips[i].getChipID() != MAX31855_ID)
    {
      Serial.print("EGT");
      Serial.print(i+1);
      Serial.println(" Error");
      delay(5000);
    }
      Serial.print("EGT");
      Serial.print(i+1);
      Serial.println(" Ok");
  }

}

void CheckDataRequest(){
       SendDataToSpeeduino(); //request ok, send the data to speeduino
}

void SendDataToSpeeduino(){
  for (int i=0; i<8; i++) {
       Serial3.write(data14[i]);
  }
  for (int i=0; i<8; i++) {
       Serial3.write(data58[i]);
  }
}

void loop() {
   for (int i=0; i<8; i++) {
     rawData[i] = MAX31855_chips[i].readRawData();
     EGT[i]  = (MAX31855_chips[i].getTemperature(rawData[i]));
     ColdJunction[i]  = (MAX31855_chips[i].getColdJunctionTemperature(rawData[i]));
     if (i < 4){
      data14[i] = lowByte(uint16_t(EGT[i]));
      data14[i+1] = highByte(uint16_t(EGT[i]));
     }
     else{
      data58[i] = lowByte(uint16_t(EGT[i]));
      data58[i+1] = highByte(uint16_t(EGT[i]));
     }
      if (Serial3.available () > 0) {  //is there data on serial3, from hopefully speeduino
        CheckDataRequest(); //is there data request from speeduino
      }
      else{ //no data request from speeduino, so broadcast to CAN bus
        //      CanSend();
      }
     Serial.print("EGT");
     Serial.print(i+1);
     Serial.print(" Temp: ");
     Serial.print(EGT[i]);
     Serial.print("Cold Junction");
     Serial.print(i+1);
     Serial.print(" Temp: ");
     Serial.print(ColdJunction[i]);
  }
}
