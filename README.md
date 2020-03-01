# 8Channel EGT-board

This is repository for 8-Channel EGT board. The board is meant to be used in conjuntion with aftermarker engine control units to 
measure exhaust gas temperatures in invidual cylinders. The PCB is based on eight MAX31855 chips and STM32F103C8T6 bluepill board.
The sensors to be used with this need to be K-type EGT sensors with mini K-type thermocouple connectors.

In order to program the bluepill, you will need FTDI breakout board and the code is meant to be used in Arduino IDE. The code works
with regular STM32 core board manager for Arduino (https://github.com/stm32duino/BoardManagerFiles/raw/master/STM32/package_stm_index.json), 
and also using this board manager too: http://dan.drown.org/stm32duino/package_STM32duino_index.json

At this moment the code is unfinished and only capable of reading the EGT sensors and sending data to speeduino through serial bus.
See the picture for settings in TS for speeduino. Support for other ecus will be added later using CAN bus.

The board is designed to be used in Hammond case number: 1455L1201 If you don't want silver case, with part number 1455L1201BK, you
get black case. 1455L1201BU gives blue case and 1455L1201RD red case. Link to manufacturers site: https://www.hammfg.com/part/1455L1201

EasyEda project link for the PCB: https://easyeda.com/pazi88/8ch_EGT
