/*

EmonLiteESP ADS1115 Example

Energy Monitor Library for ESP8266 based on EmonLib
Currently only support current sensing

NOTE: Requires Adafruit_ADS1015 library

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
#include <Wire.h>
#include <Adafruit_ADS1015.h>

// -----------------------------------------------------------------------------
// Configuration
// -----------------------------------------------------------------------------

// The ADC input range (or gain) can be changed via the following
// functions, but be careful never to exceed VDD +0.3V max, or to
// exceed the upper and lower limits if you adjust the input range!
// Setting these values incorrectly may destroy your ADC!
//                                                                ADS1015  ADS1115
//                                                                -------  -------
// ads.setGain(GAIN_TWOTHIRDS);  // 2/3x gain +/- 6.144V  1 bit = 3mV      0.1875mV (default)
// ads.setGain(GAIN_ONE);        // 1x gain   +/- 4.096V  1 bit = 2mV      0.125mV
// ads.setGain(GAIN_TWO);        // 2x gain   +/- 2.048V  1 bit = 1mV      0.0625mV
// ads.setGain(GAIN_FOUR);       // 4x gain   +/- 1.024V  1 bit = 0.5mV    0.03125mV
// ads.setGain(GAIN_EIGHT);      // 8x gain   +/- 0.512V  1 bit = 0.25mV   0.015625mV
// ads.setGain(GAIN_SIXTEEN);    // 16x gain  +/- 0.256V  1 bit = 0.125mV  0.0078125mV
#define ADS1115_GAIN            GAIN_FOUR

// If you are using a nude ESP8266 board it will be 1.0V
// If using a NodeMCU there is a voltage divider in place, so use 3.3V instead.
// If using an ADS1115 depends on gain factor, check table and duplicate (only +)
#define REFERENCE_VOLTAGE       2.048

// Precision of the ADC measure in bits. Arduinos and ESP8266 use 10bits ADCs.
// The ADS1115 is a 16bits ADC
#define ADC_BITS                16

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

Adafruit_ADS1115 ads;
EmonLiteESP power1;
EmonLiteESP power2;
EmonLiteESP power3;

// -----------------------------------------------------------------------------
// Energy Monitor
// -----------------------------------------------------------------------------

void powerMonitorSetup() {

    // ADS1115 setup
    ads.setGain(ADS1115_GAIN);
    ads.begin();

    // Setup line 1
    power1.initCurrent([]() -> unsigned int {
        return ads.readADC_SingleEnded(0);
    }, ADC_BITS, REFERENCE_VOLTAGE, CURRENT_RATIO);

    // Setup line 2
    power2.initCurrent([]() -> unsigned int {
        return ads.readADC_SingleEnded(1);
    }, ADC_BITS, REFERENCE_VOLTAGE, CURRENT_RATIO);

    // Setup line 1
    power3.initCurrent([]() -> unsigned int {
        return ads.readADC_SingleEnded(2);
    }, ADC_BITS, REFERENCE_VOLTAGE, CURRENT_RATIO);
}

void powerMonitorLoop() {

    static unsigned long last_check = 0;

    if ((millis() - last_check) > MEASUREMENT_INTERVAL) {

        double current;

        current = power1.getCurrent(SAMPLES_X_MEASUREMENT);
        Serial.printf("[ENERGY] Power now on line #1: %dW\n", int(current * MAINS_VOLTAGE));

        current = power2.getCurrent(SAMPLES_X_MEASUREMENT);
        Serial.printf("[ENERGY] Power now on line #2: %dW\n", int(current * MAINS_VOLTAGE));

        current = power3.getCurrent(SAMPLES_X_MEASUREMENT);
        Serial.printf("[ENERGY] Power now on line #3: %dW\n", int(current * MAINS_VOLTAGE));

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
