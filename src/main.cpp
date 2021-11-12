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
int sweepIncrementRpm = 25;
int sweepStartRpm = 500;
int sweepStopRpm = 6000;
int step;
int multipliedRpm;
int currentRpm = sweepStartRpm;
int currentTempCelsius;
int fuelConsumption = 0;

// Define CAN payloads for each use case
unsigned char canPayloadRpm[8] = {0, 0, 0, 0, 0, 0, 0, 0};     //RPM
unsigned char canPayloadTemp[8] = {0, 0, 0, 0, 0, 0, 0, 0};    //Temp
unsigned char canPayloadMisc[8] = {0, 0, 0, 0, 0, 0, 0, 0};    //Misc

// Function for reading RPM value
void canReadRpm(){
    if (currentRpm >= sweepStopRpm) {
        step = -sweepIncrementRpm;
    } else if (currentRpm <= sweepStartRpm) {
        step = sweepIncrementRpm;
    } 

    currentRpm = currentRpm + step;
    multipliedRpm = currentRpm * rpmHexConversionMultipler;
}

// Function for sending RPM payload
void canWriteRpm(){
    rpmHexConversionMultipler = (-0.00005540102040816370 * currentRpm) + 6.70061224489796;

    canPayloadRpm[2] = multipliedRpm;            //LSB
    canPayloadRpm[3] = (multipliedRpm >> 8);     //MSB

    CAN.sendMsgBuf(0x316, 0, 8, canPayloadRpm);
}

// Function for reading temp value
void canReadTemp(){
    currentTempCelsius = 0; // Dummy value for development to replace later, 143 seems to be the maximum
}

// Function for sending temp payload
void canWriteTemp(){
    canPayloadTemp[1] = (currentTempCelsius + 48.373) / 0.75;
    CAN.sendMsgBuf(0x329, 0, 8, canPayloadTemp);
}

// Function for testing misc functionality
void canWriteMisc() {
    canPayloadMisc[0] = 2;                  // 2 for check engine light
                                            // 16 for EML light
                                            // 18 for check engine AND EML (add together)
    canPayloadMisc[2] = fuelConsumption;    // Fuel consumption, rate of change drives value of gauge
    canPayloadMisc[3] = 8;                  // 8 Over heat light

    fuelConsumption += 10;

    CAN.sendMsgBuf(0x545, 0, 8, canPayloadMisc);
}

// Define our timed actions
TimedAction readRpmThread = TimedAction(40,canReadRpm);
TimedAction readTempThread = TimedAction(40,canReadTemp);
TimedAction writeRpmThread = TimedAction(40,canWriteRpm);
TimedAction writeTempThread = TimedAction(40,canWriteTemp);
TimedAction writeMiscThread = TimedAction(50,canWriteMisc);

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
    readRpmThread.check();
    writeRpmThread.check();
    readTempThread.check();
    writeTempThread.check();
    writeMiscThread.check();
}
