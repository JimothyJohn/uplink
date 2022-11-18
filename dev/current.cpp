/*
* @file readACCurrent.
* @n This example reads Analog AC Current Sensor.
* @copyright Copyright (c) 2010 DFRobot Co.Ltd (https://www.dfrobot.com)
* @licence The MIT License (MIT)
* @get from https://www.dfrobot.com
Created 2016-3-10
By berinie Chen <bernie.chen@dfrobot.com>
Revised 2019-8-6
By Henry Zhao<henry.zhao@dfrobot.com>
Adapted 2022-4-16
By Nick Armenta<nick@advin.io>
// https://github.com/JimothyJohn/current-monitor/blob/master/src/current.cpp
*/

#include <Arduino.h>
const uint8_t ACPin = 0;   // set arduino signal read pin
#define ACTectionRange 20; // set Non-invasive AC Current Sensor tection range

// VREF: Analog reference
// For Arduino UNO, Leonardo and mega2560, etc. change VREF to 5
// For Arduino Zero, Due, MKR Family, ESP32, etc. 3V3 controllers, change VREF to 3.3
#define VREF 3.3
#define FS 100
int sampleDelay = static_cast<int>(1000 / FS);
float readACCurrentValue()
{
    float ACCurrtntValue = 0;
    float peakVoltage = 0;
    float voltageVirtualValue = 0; // Vrms
    for (int i = 0; i < 5; i++)
    {
        // read peak voltage
        peakVoltage += analogRead(ACPin);
        delay(sampleDelay);
    }
    peakVoltage = peakVoltage / 5;
    // change the peak voltage to the Virtual Value of voltage
    voltageVirtualValue = peakVoltage * 0.707;
    /*The circuit is amplified by 2 times, so it is divided by 2.*/
    voltageVirtualValue = (voltageVirtualValue / 1024 * VREF) / 2;
    ACCurrtntValue = voltageVirtualValue * ACTectionRange;
    return ACCurrtntValue;
}

void setup()
{
    // Boot-up delay
    delay(5000);
    Serial.begin(115200);
    while (!Serial)
        ;
    Serial.println("Current");
    pinMode(13, OUTPUT);
}

void loop()
{
    float ACCurrentValue = readACCurrentValue(); // read AC Current Value
    // Edge Impulse requires .csv format
    // https://docs.edgeimpulse.com/docs/edge-impulse-cli/cli-data-forwarder#protocol
    Serial.println(ACCurrentValue);
    // 50ms is ~18Hz, the maximum sample rate of Edge Impulse
    delay(50);
}
