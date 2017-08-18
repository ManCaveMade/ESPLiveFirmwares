/*

EmonLiteESP ADC121 Example using Brzo I2C library

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
#include "brzo_i2c.h"

// -----------------------------------------------------------------------------
// Configuration
// -----------------------------------------------------------------------------

// I2C CONFIGURATION
#define I2C_SDA_PIN             4
#define I2C_SCL_PIN             14
#define I2C_CLOCK_STRETCH_TIME  200
#define I2C_SCL_FREQUENCY       1000

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
    brzo_i2c_setup(I2C_SDA_PIN, I2C_SCL_PIN, I2C_CLOCK_STRETCH_TIME);

    // Set the ADC121 fo manual readings (no automatic sampling)
    uint8_t buffer[2];
    buffer[0] = ADC121_REG_CONFIG;
    buffer[1] = 0x00;
    brzo_i2c_start_transaction(ADC121_ADDRESS, I2C_SCL_FREQUENCY);
    brzo_i2c_write(buffer, 2, false);
    brzo_i2c_end_transaction();

    // Setup power monitor
    monitor.initCurrent([]() -> unsigned int {

        unsigned int value;
        uint8_t buffer[2];

        // Ask for a reading
        brzo_i2c_start_transaction(ADC121_ADDRESS, I2C_SCL_FREQUENCY);
        buffer[0] = ADC121_REG_RESULT;
        brzo_i2c_write(buffer, 1, false);

        // Read the value
        brzo_i2c_read(buffer, 2, false);
        brzo_i2c_end_transaction();
        value = (buffer[0] & 0x0F) << 8;
        value |= buffer[1];

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
