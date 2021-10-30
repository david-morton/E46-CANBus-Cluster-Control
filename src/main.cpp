/* This is a very basic program to drive the E46 gauge cluster via CAN. The hardware used
was a Arduino Mega 2560 r3 and Seeed CAN-Bus shield v2. The program initially started life
as the 'send' example from the Seeed shield library examples.

The scaling ratio of engine RPM to hex value (bytes 2 and 3) of CAN payload is 
approximated in this program based on a linear formula worked out in Excel.

For reference the measured ratios at varios RPM's in my own care are as per the below:
1000 6.65
2000 6.59
3000 6.53
4000 6.48
5000 6.42
6000 6.37
7000 6.31
*/

// TODO
// Temp gauge warning light
// Try to sort economy gauge to be something more useful (oil pressure etc)

#include <SPI.h>
#include <TimedAction.h>

#define CAN_2515

const int SPI_CS_PIN = 9;
const int CAN_INT_PIN = 2;

#include "mcp2515_can.h"
mcp2515_can CAN(SPI_CS_PIN); // Set CS pin

// Define varaibles that will be used later
float rpmHexConversionMultipler = 6.55; // Accurate multiplier at lower RPM in case calculation is too heavy
int sweepIncrementRpm = 10;
int sweepStartRpm = 1000;
int sweepStopRpm = 4000;
int step;
int multipliedRpm;
int currentRpm = sweepStartRpm;

// Define CAN payloads for each use case
unsigned char stmp[8] = {0, 0, 0, 0, 0, 0, 0, 0};     //RPM
unsigned char stmp2[8] = {0, 0xAB, 0, 0, 0, 0, 0, 0}; //Temp

// Function for sending RPM payload
void canSendRpm(){
    if (currentRpm >= sweepStopRpm) {
        step = -sweepIncrementRpm;
    } else if (currentRpm <= sweepStartRpm) {
        step = sweepIncrementRpm;
    }

    rpmHexConversionMultipler = (-0.00005540102040816370 * currentRpm) + 6.70061224489796;

    currentRpm = currentRpm + step;
    multipliedRpm = currentRpm * rpmHexConversionMultipler;
        
    stmp[2] = multipliedRpm;            //LSB
    stmp[3] = (multipliedRpm >> 8);     //MSB

    CAN.sendMsgBuf(0x316, 0, 8, stmp);
}

// Function for sending temp payload
void canSendTemp(){
    CAN.sendMsgBuf(0x329, 0, 8, stmp2);
}

// Define our timed actions
TimedAction rpmThread = TimedAction(40,canSendRpm);
TimedAction tempThread = TimedAction(40,canSendTemp);

void setup() {
    SERIAL_PORT_MONITOR.begin(115200);
    while(!Serial){};

    while (CAN_OK != CAN.begin(CAN_500KBPS)) {             // init can bus : baudrate = 500k
        SERIAL_PORT_MONITOR.println("CAN init fail, retry...");
        delay(250);
    }
    SERIAL_PORT_MONITOR.println("CAN init ok!");
}

void loop() {
    rpmThread.check();
    tempThread.check();
}
