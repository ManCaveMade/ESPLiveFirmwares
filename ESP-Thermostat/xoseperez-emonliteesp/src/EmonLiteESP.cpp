/*

EmonLiteESP 0.1.1

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

#include "Arduino.h"
#include "EmonLiteESP.h"

void EmonLiteESP::initCurrent(current_c callback, unsigned char bits, double ref, double ratio) {

    _currentCallback = callback;
    _adcBits = bits;
    _adcCounts = 1 << bits;
    _referenceVoltage = ref;
    _currentRatio = ratio;
    _currentMidPoint = (_adcCounts>>1);
    calculateMultiplier();

};

void EmonLiteESP::warmup() {
    int sample;
    for (unsigned int n = 0; n < WARMUP_COUNTS; n++) {
        sample = _currentCallback();
        _currentMidPoint = (_currentMidPoint + (sample - _currentMidPoint) / _adcCounts);
    }
}

void EmonLiteESP::calculateMultiplier() {

    _currentFactor = _currentRatio * _referenceVoltage / _adcCounts;
    unsigned int s = 1;
    unsigned int i = 1;
    unsigned int m = s * i;
    while (m * _currentFactor < 1) {
        _multiplier = m;
        i = (i == 1) ? 2 : (i == 2) ? 5 : 1;
        if (i == 1) s *= 10;
        m = s * i;
    }

    /*
    Serial.print("[EMON] Current ratio: "); Serial.println(_currentRatio);
    Serial.print("[EMON] Ref. Voltage: "); Serial.println(_referenceVoltage);
    Serial.print("[EMON] ADC Couns: "); Serial.println(_adcCounts);
    Serial.print("[EMON] Current factor: "); Serial.println(_currentFactor);
    Serial.print("[EMON] Multiplier: "); Serial.println(_multiplier);
    */

}

void EmonLiteESP::setReference(double ref) {
    _referenceVoltage = ref;
}

void EmonLiteESP::setCurrentRatio(double ratio) {
    _currentRatio = ratio;
    calculateMultiplier();
}

unsigned int EmonLiteESP::getMultiplier() {
    return _multiplier;
}

void EmonLiteESP::setMultiplier(unsigned int multiplier) {
    _multiplier = multiplier;
}

double EmonLiteESP::getCurrent(unsigned int samples) {

    int sample;
    double filtered;
    double sum;
    //int max = 0;
    //int min = _adcCounts;

    unsigned long start = millis();
    for (unsigned int n = 0; n < samples; n++) {

        // Read analog value
        sample = _currentCallback();
        //if (sample > max) max = sample;
        //if (sample < min) min = sample;

        // Digital low pass filter extracts the VDC offset
        _currentMidPoint = (_currentMidPoint + (sample - _currentMidPoint) / _adcCounts);
        filtered = sample - _currentMidPoint;

        // Root-mean-square method
        sum += (filtered * filtered);

    }

    double rms = samples > 0 ? sqrt(sum / samples) : 0;
    double current = _currentFactor * rms;
    current = int(current * _multiplier) / _multiplier;

    /*
    Serial.print("[EMON] Total time (ms): "); Serial.println(millis() - start);
    Serial.print("[EMON] Sample frequency (1/s): "); Serial.println(1000 * samples / (millis() - start));
    Serial.print("[EMON] Max value: "); Serial.println(max);
    Serial.print("[EMON] Min value: "); Serial.println(min);
    Serial.print("[EMON] Midpoint value: "); Serial.println(_currentMidPoint);
    Serial.print("[EMON] RMS value: "); Serial.println(rms);
    Serial.print("[EMON] Current: "); Serial.println(current);
    */

    return current;

};
