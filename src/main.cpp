/* This is a basic program to drive the E46 gauge cluster via CAN. The hardware used
was a single Arduino Mega 2560 r3 and two Seeed CAN-Bus shield v2's.

This project was used in the context of an engine swap where a Nissan VQ37VHR was
installed into the E46, and the desire was to maintain a functional OEM dashboard.

The scaling ratio of engine RPM to the hex value of CAN payload is 
approximated in this program based on a linear formula worked out in Excel.
This was done from a test cluster so that visually the needle lined up
as well as possible throughout the RPM range. Individual clusters may vary
so some experimentation with this may be desired ... practically speaking it
may make no differance at all :)

For reference the measured ratios at varios RPM's in my own test cluster are as per the below:
1000 6.65
2000 6.59
3000 6.53
4000 6.48
5000 6.42
6000 6.37
7000 6.31

The fuel economy gauge only works when there is a speed input. Inputting a square wave of
410Hz and 50% duty cycle gives 60km/h. This signal is put into pin 19 on cluster connector
X11175 for testing purposes on the bench. This signal is usually provided by the ABS computer.

The lowest speed pulse generation that seems to allow activation of the fuel economy gauge is
82Hz when increasing and it will stop function at 68Hz when decreasing. 82Hz seems to be about
5kph.
*/

#include <SPI.h>
#include <TimedAction.h>
#include <mcp2515_can.h>

#define CAN_2515

const int SPI_SS_PIN_BMW = 9;              // Slave select pin for CAN shield 1 (BMW CAN bus)
const int SPI_SS_PIN_NISSAN = 10;          // Slave select pin for CAN shield 2 (Nissan CAN bus)
const int CAN_INT_PIN = 2;

mcp2515_can CAN_BMW(SPI_SS_PIN_BMW);
mcp2515_can CAN_NISSAN(SPI_SS_PIN_NISSAN);

// Define varaibles that will be used later
float rpmHexConversionMultipler = 6.55;    // Default multiplier set to a sensible value for accuracy at lower RPM.
                                           // This will be overriden via the formula based multiplier later on if used.
const int sweepIncrementRpm = 25;
const int sweepStartRpm = 1000;
const int sweepStopRpm = 3000;
const int tempAlarmLight = 110;

int step;
int multipliedRpm;
int currentRpm = sweepStartRpm;
int currentTempCelsius;

// Define CAN payloads for each use case
unsigned char canPayloadRpm[8] = {0, 0, 0, 0, 0, 0, 0, 0};     //RPM
unsigned char canPayloadTemp[8] = {0, 0, 0, 0, 0, 0, 0, 0};    //Temp
unsigned char canPayloadMisc[8] = {0, 0, 0, 0, 0, 0, 0, 0};    //Misc (check light, consumption and temp alarm light)

// Function - Read RPM value from Nissan CAN
void canReadRpm(){
    if (currentRpm >= sweepStopRpm)
        step = -sweepIncrementRpm;
    else if (currentRpm <= sweepStartRpm)
        step = sweepIncrementRpm;

    currentRpm = currentRpm + step;
    multipliedRpm = currentRpm * rpmHexConversionMultipler;
}

// Function - Write RPM value to BMW CAN
void canWriteRpm(){
    rpmHexConversionMultipler = (-0.00005540102040816370 * currentRpm) + 6.70061224489796;

    canPayloadRpm[2] = multipliedRpm;            //LSB
    canPayloadRpm[3] = (multipliedRpm >> 8);     //MSB

    CAN_BMW.sendMsgBuf(0x316, 0, 8, canPayloadRpm);
}

// Function - Read temp value from Nissan CAN
void canReadTemp(){
    currentTempCelsius = 95;
}

// Function - Write temp value to BMW CAN
void canWriteTemp(){
    canPayloadTemp[1] = (currentTempCelsius + 48.373) / 0.75;
    CAN_BMW.sendMsgBuf(0x329, 0, 8, canPayloadTemp);
}

int consumptionCounter = 0;
int consumptionIncrease = 40;
int consumptionValue = 0;

// Function - Write misc payload to BMW CAN
void canWriteMisc() {
    canPayloadMisc[0] = 0;                          // 2 for check engine light
                                                    // 16 for EML light
                                                    // 18 for check engine AND EML (add together)
    canPayloadMisc[1] = consumptionValue;           // Fuel consumption LSB                                        
    canPayloadMisc[2] = (consumptionValue >> 8);    // Fuel consumption MSB
    if (currentTempCelsius >= tempAlarmLight)       // Set the red alarm light on the temp gauge if needed
        canPayloadMisc[3] = 8;
    else
        canPayloadMisc[3] = 0;

    if (consumptionCounter % 1 == 0) {
        consumptionValue += consumptionIncrease;
    }

    consumptionCounter++;

    CAN_BMW.sendMsgBuf(0x545, 0, 8, canPayloadMisc);
}

// Define our timed actions
TimedAction readRpmThread = TimedAction(40,canReadRpm);
TimedAction readTempThread = TimedAction(40,canReadTemp);
TimedAction writeRpmThread = TimedAction(10,canWriteRpm);
TimedAction writeTempThread = TimedAction(10,canWriteTemp);
TimedAction writeMiscThread = TimedAction(10,canWriteMisc);

// Our main setup stanza
void setup() {
    SERIAL_PORT_MONITOR.begin(115200);
    while(!Serial){};

    // Configure CAN interfaces
    while (CAN_OK != CAN_BMW.begin(CAN_500KBPS)) {             // init can bus : baudrate = 500k
        SERIAL_PORT_MONITOR.println("BMW CAN init fail, retry...");
        delay(250);
    }
    SERIAL_PORT_MONITOR.println("BMW CAN init ok!");

    while (CAN_OK != CAN_NISSAN.begin(CAN_500KBPS)) {          // init can bus : baudrate = 500k ??
        SERIAL_PORT_MONITOR.println("Nissan CAN init fail, retry...");
        delay(250);
    }
    SERIAL_PORT_MONITOR.println("Nissan CAN init ok!");
}

// Our main loop
void loop() {
    readRpmThread.check();
    writeRpmThread.check();
    readTempThread.check();
    writeTempThread.check();
    writeMiscThread.check();
}
