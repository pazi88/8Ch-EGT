# 8Channel EGT-board

This is repository for 8-Channel EGT board. The board is meant to be used in conjuntion with aftermarker engine control units to 
measure exhaust gas temperatures in invidual cylinders. The PCB is based on eight MAX31855 chips and STM32F103C8T6 based bluepill board.
The sensors to be used with this need to be K-type EGT sensors with mini K-type thermocouple connectors.

In order to program the bluepill, you will need FTDI breakout board and the code is meant to be used in Arduino IDE. The code should work
with regular STM32 core for Arduino, but I had no luck programming it to the bluepill. But using this board manager, the code uploading
works: http://dan.drown.org/stm32duino/package_STM32duino_index.json

At this moment the code is unfinished and only capable of reading the EGT sensors. But it should be soon capable of sending data to
speeduino by serial bus. Support for other ecus will be added using CAN bus.
