#include "MAX31855.h"
#ifdef ARDUINO_BLUEPILL_F103C8
HardwareSerial Serial3(USART3); //for some reason this isn't defined in arduino_core_stm32
#endif

#define Sensor1_4_CAN_ADDRESS   0x20A //the data from sensers 1-4 will be sent using this address
#define Sensor5_8_CAN_ADDRESS   0x20B //the data from sensers 5-8 will be sent using this address

enum BITRATE{CAN_50KBPS, CAN_100KBPS, CAN_125KBPS, CAN_250KBPS, CAN_500KBPS, CAN_1000KBPS};

typedef struct
{
  uint16_t id;
  uint8_t  data[8];
  uint8_t  len;
} CAN_msg_t;

CAN_msg_t CAN_TX_msg;

typedef const struct
{
  uint8_t TS2;
  uint8_t TS1;
  uint8_t BRP;
} CAN_bit_timing_config_t;
CAN_bit_timing_config_t can_configs[6] = {{2, 13, 45}, {2, 15, 20}, {2, 13, 18}, {2, 13, 9}, {2, 15, 4}, {2, 15, 2}};

extern CAN_bit_timing_config_t can_configs[6];

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

void CANInit(enum BITRATE bitrate)
 {
    RCC->APB1ENR |= 0x2000000UL;      // Enable CAN clock 
    RCC->APB2ENR |= 0x1UL;            // Enable AFIO clock
    AFIO->MAPR   &= 0xFFFF9FFF;       // reset CAN remap
    AFIO->MAPR   |= 0x00000000;       //  et CAN remap, use PA11, PA12
 
    RCC->APB2ENR |= 0x4UL;            // Enable GPIOA clock (bit2 to 1)
    GPIOA->CRH   &= 0xFFF00FFF;
    GPIOA->CRH   |= 0xB8000UL;            // Configure PA11 and PA12
    GPIOA->ODR   |= 0x1000UL;
  
    CAN1->MCR = 0x51UL;                // Set CAN to initialization mode
     
    // Set bit rates 
    CAN1->BTR &= ~(((0x03) << 24) | ((0x07) << 20) | ((0x0F) << 16) | (0x1FF)); 
    CAN1->BTR |=  (((can_configs[bitrate].TS2-1) & 0x07) << 20) | (((can_configs[bitrate].TS1-1) & 0x0F) << 16) | ((can_configs[bitrate].BRP-1) & 0x1FF);
 
    // Configure Filters to default values
    CAN1->FM1R |= 0x1C << 8;              // Assign all filters to CAN1
    CAN1->FMR  |=   0x1UL;                // Set to filter initialization mode
    CAN1->FA1R &= ~(0x1UL);               // Deactivate filter 0
    CAN1->FS1R |=   0x1UL;                // Set first filter to single 32 bit configuration
 
    CAN1->sFilterRegister[0].FR1 = 0x0UL; // Set filter registers to 0
    CAN1->sFilterRegister[0].FR2 = 0x0UL; // Set filter registers to 0
    CAN1->FM1R &= ~(0x1UL);               // Set filter to mask mode
 
    CAN1->FFA1R &= ~(0x1UL);              // Apply filter to FIFO 0  
    CAN1->FA1R  |=   0x1UL;               // Activate filter 0
    
    CAN1->FMR   &= ~(0x1UL);              // Deactivate initialization mode
    CAN1->MCR   &= ~(0x1UL);              // Set CAN to normal mode 

    while (CAN1->MSR & 0x1UL); 
 
 }

void setup() {
  MAX31855_OK_bits = 0;
  Serial.begin(115200); //debug
  Serial3.begin(115200); //data to speeduino
  CANInit(CAN_500KBPS); //init can at 500KBPS speed
  CAN_TX_msg.len = 8; //8 bytes in can message

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

void CANSend(CAN_msg_t* CAN_tx_msg)
 {
    volatile int count = 0;
     
    CAN1->sTxMailBox[0].TIR   = (CAN_tx_msg->id) << 21;
    
    CAN1->sTxMailBox[0].TDTR &= ~(0xF);
    CAN1->sTxMailBox[0].TDTR |= CAN_tx_msg->len & 0xFUL;
    
    CAN1->sTxMailBox[0].TDLR  = (((uint32_t) CAN_tx_msg->data[3] << 24) |
                                 ((uint32_t) CAN_tx_msg->data[2] << 16) |
                                 ((uint32_t) CAN_tx_msg->data[1] <<  8) |
                                 ((uint32_t) CAN_tx_msg->data[0]      ));
    CAN1->sTxMailBox[0].TDHR  = (((uint32_t) CAN_tx_msg->data[7] << 24) |
                                 ((uint32_t) CAN_tx_msg->data[6] << 16) |
                                 ((uint32_t) CAN_tx_msg->data[5] <<  8) |
                                 ((uint32_t) CAN_tx_msg->data[4]      ));

    CAN1->sTxMailBox[0].TIR  |= 0x1UL;
    while(CAN1->sTxMailBox[0].TIR & 0x1UL && count++ < 1000000);
     
     if (!(CAN1->sTxMailBox[0].TIR & 0x1UL)) return;
     
     //Sends error log to screen
     while (CAN1->sTxMailBox[0].TIR & 0x1UL)
     {
         Serial.println(CAN1->ESR);        
         Serial.println(CAN1->MSR);        
         Serial.println(CAN1->TSR);
        
     }
 }

void loop() {
   for (int i=0; i<8; i++) {
    if (bitRead(MAX31855_OK_bits, i) == 1){
     rawData[i] = MAX31855_chips[i].readRawData();
     EGT[i]  = (MAX31855_chips[i].getTemperature(rawData[i]));
     ColdJunction[i]  = (MAX31855_chips[i].getColdJunctionTemperature(rawData[i]));
     if (i < 4){
      if (EGT[i] < 2001){ //just in case filter if MAX31855 gives faulty value. 2000c is maximum that it gives out.
        data14[2*i] = lowByte(uint16_t(EGT[i]));
        data14[2*i+1] = highByte(uint16_t(EGT[i]));
      }
     }
     else{
      if (EGT[i] < 2001){
        data58[2*i-8] = lowByte(uint16_t(EGT[i]));
        data58[2*i-7] = highByte(uint16_t(EGT[i]));
      }
     }
      while (Serial3.available () > 0) {  //is there data on serial3, presumably from speeduino
        CheckDataRequest(); //there is data, but is it request from speeduino and is it for EGTs
      }
        if (i < 4){
         CAN_TX_msg.data[2*i] = data14[2*i];
         CAN_TX_msg.data[2*i+1] = data14[2*i+1];
         CAN_TX_msg.id = Sensor1_4_CAN_ADDRESS;
         CANSend(&CAN_TX_msg);
       }
       else{
         CAN_TX_msg.data[2*i-8] = data58[2*i-8];
         CAN_TX_msg.data[2*i-7] = data58[2*i-7];
         CAN_TX_msg.id = Sensor5_8_CAN_ADDRESS;
         CANSend(&CAN_TX_msg);
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
