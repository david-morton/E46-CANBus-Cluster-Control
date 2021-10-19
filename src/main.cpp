/* This is a very basic RPM gauge sweep for the E46 gauge cluster.
Hardware used was a clone Arduino Mega 2560 r3 and Seeed CAN-Bus shield v2
Program initially based on 'send' from Arduino examples from Seeed CAN-Bus shield library

The scaling ratio of engine RPM to hex value (bytes 2 and 3) of CAN payload is 
approximated in this program based on a linear formula. 

The measured ratios at varios RPM's are as per the below (included for reference):
1000 6.65
2000 6.59
3000 6.53
4000 6.48
5000 6.42
6000 6.37
7000 6.31
*/

#include <SPI.h>

#define CAN_2515

const int SPI_CS_PIN = 9;
const int CAN_INT_PIN = 2;

#include "mcp2515_can.h"
mcp2515_can CAN(SPI_CS_PIN); // Set CS pin

void setup() {
    SERIAL_PORT_MONITOR.begin(115200);
    while(!Serial){};

    while (CAN_OK != CAN.begin(CAN_500KBPS)) {             // init can bus : baudrate = 500k
        SERIAL_PORT_MONITOR.println("CAN init fail, retry...");
        delay(250);
    }
    SERIAL_PORT_MONITOR.println("CAN init ok!");
}

float rpmHexConversionMultipler = 6.6; // Accurate multiplier at lower RPM in case calculation is too heavy
int sweepIncrementRpm = 100;
int sweepStartRpm = 1000;
int sweepStopRpm = 7000;
int sweepDelayMs = 50;
int step;
int multipliedRpm;
int currentRpm = sweepStartRpm;

unsigned char stmp[8] = {0, 0, 0, 0, 0, 0, 0, 0};     //RPM
unsigned char stmp2[8] = {0, 0xAB, 0, 0, 0, 0, 0, 0}; //Temp

void loop() {
    if (currentRpm >= sweepStopRpm) {
        step = -sweepIncrementRpm;
    } else if (currentRpm <= sweepStartRpm) {
        step = sweepIncrementRpm;
    }

    rpmHexConversionMultipler = (-0.00005540102040816370 * currentRpm) + 6.70061224489796;

    // SERIAL_PORT_MONITOR.println(rpmHexConversionMultipler);

    currentRpm = currentRpm + step;
    multipliedRpm = currentRpm * rpmHexConversionMultipler;
        
    stmp[2] = multipliedRpm;            //LSB
    stmp[3] = (multipliedRpm >> 8);     //MSB

    CAN.sendMsgBuf(0x316, 0, 8, stmp);
    CAN.sendMsgBuf(0x329, 0, 8, stmp2);
    delay(sweepDelayMs);
}
