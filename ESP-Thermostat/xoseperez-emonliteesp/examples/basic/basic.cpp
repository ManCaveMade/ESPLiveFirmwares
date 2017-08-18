/*

EmonLiteESP Basic Example

Energy Monitor Library for ESP8266 based on EmonLib
Currently only support current sensing

Copyright (C) 2016 by Xose Pérez <xose dot perez at gmail dot com>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/


#include <Arduino.h>
#include "EmonLiteESP.h"

// -----------------------------------------------------------------------------
// Configuration
// -----------------------------------------------------------------------------

// Aanalog GPIO on the ESP8266
#define CURRENT_PIN             0

// If you are using a nude ESP8266 board it will be 1.0V, if using a NodeMCU there
// is a voltage divider in place, so use 3.3V instead.
#define REFERENCE_VOLTAGE       1.0

// Precision of the ADC measure in bits. Arduinos and ESP8266 use 10bits ADCs, but the
// ADS1115 is a 16bits ADC
#define ADC_BITS                10

// This is basically the volts per amper ratio of your current measurement sensor.
// If your sensor has a voltage output it will be written in the sensor enclosure,
// something like "30V 1A", otherwise it will depend on the burden resistor you are
// using.
#define CURRENT_RATIO           30

// This version of the library only calculate aparent power, so it asumes a fixes
// mains voltage
#define MAINS_VOLTAGE           230

// Number of samples each time you measure
#define SAMPLES_X_MEASUREMENT   1000

// Time between readings, this is not specific of the library but on this sketch
#define MEASUREMENT_INTERVAL    10000

// -----------------------------------------------------------------------------
// Globals
// -----------------------------------------------------------------------------

EmonLiteESP power;

// -----------------------------------------------------------------------------
// Energy Monitor
// -----------------------------------------------------------------------------

unsigned int currentCallback() {

    // If usingthe ADC GPIO in the ESP8266
    return analogRead(CURRENT_PIN);

}

void powerMonitorSetup() {
    power.initCurrent(currentCallback, ADC_BITS, REFERENCE_VOLTAGE, CURRENT_RATIO);
}

void powerMonitorLoop() {

    static unsigned long last_check = 0;

    if ((millis() - last_check) > MEASUREMENT_INTERVAL) {

        double current = power.getCurrent(SAMPLES_X_MEASUREMENT);

        Serial.print(F("[ENERGY] Power now: "));
        Serial.print(int(current * MAINS_VOLTAGE));
        Serial.println(F("W"));

        last_check = millis();

    }
}


// -----------------------------------------------------------------------------
// Main methods
// -----------------------------------------------------------------------------

void setup() {
    Serial.begin(115200);
    powerMonitorSetup();
}

void loop() {
    powerMonitorLoop();
    delay(1);
}
