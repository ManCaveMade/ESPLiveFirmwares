/*

EmonLiteESP ADC121 Example

Energy Monitor Library for ESP8266 based on EmonLib
Currently only support current sensing

Copyright (C) 2017 by Xose Pérez <xose dot perez at gmail dot com>

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

// -----------------------------------------------------------------------------
// Configuration
// -----------------------------------------------------------------------------

// I2C PINS
#define I2C_SDA_PIN             4
#define I2C_SCL_PIN             14

// ADC121 Address
#define ADC121_ADDRESS          0x50

// ADC121 Registers
#define ADC121_REG_RESULT       0x00
#define ADC121_REG_ALERT        0x01
#define ADC121_REG_CONFIG       0x02
#define ADC121_REG_LIMITL       0x03
#define ADC121_REG_LIMITH       0x04
#define ADC121_REG_HYST         0x05
#define ADC121_REG_CONVL        0x06
#define ADC121_REG_CONVH        0x07

// If you are using a nude ESP8266 board it will be 1.0V
// If using a NodeMCU there is a voltage divider in place, so use 3.3V instead.
#define REFERENCE_VOLTAGE       3.3

// Precision of the ADC measure in bits. Arduinos and ESP8266 use 10bits ADCs.
// The ADC121 is a 12bits ADC
#define ADC_BITS                12

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

EmonLiteESP monitor;

// -----------------------------------------------------------------------------
// Energy Monitor
// -----------------------------------------------------------------------------

void powerMonitorSetup() {

    // Init I2C protocol
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);

    // Set the ADC121 fo manual readings (no automatic sampling)
    Wire.beginTransmission(ADC121_ADDRESS);
    Wire.write(ADC121_REG_CONFIG);
    Wire.write(0x00);
    Wire.endTransmission();

    // Setup power monitor
    monitor.initCurrent([]() -> unsigned int {

        unsigned int value;

        // Ask for a reading
        Wire.beginTransmission(ADC121_ADDRESS);
        Wire.write(ADC121_REG_RESULT);
        Wire.endTransmission();

        // Get value (12 bits in 2 bytes)
        Wire.requestFrom(ADC121_ADDRESS, 2);
        if (Wire.available() <= 2) {
            value = (Wire.read() & 0x0F) << 8;
            value |= Wire.read();
        }

        return value;

    }, ADC_BITS, REFERENCE_VOLTAGE, CURRENT_RATIO);

}

void powerMonitorLoop() {

    static unsigned long last_check = 0;

    if ((millis() - last_check) > MEASUREMENT_INTERVAL) {

        unsigned long start = millis();
        double current = monitor.getCurrent(SAMPLES_X_MEASUREMENT);
        Serial.printf("[ENERGY] Sampling time: %ldms\n", millis() - start);
        Serial.printf("[ENERGY] Power now: %dW\n", int(current * MAINS_VOLTAGE));

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
