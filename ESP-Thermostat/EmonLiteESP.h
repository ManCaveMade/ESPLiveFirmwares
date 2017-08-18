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

#ifndef EmonLiteESP_h
#define EmonLiteESP_h

#include <Arduino.h>

#define WARMUP_COUNTS   3000

typedef unsigned int (*current_c)();

class EmonLiteESP {

  public:

    void initCurrent(current_c callback, unsigned char bits, double ref, double ratio);
    double getCurrent(unsigned int samples);
    unsigned int getMultiplier();
    void setMultiplier(unsigned int multiplier);
    void setReference(double ref);
    void setCurrentRatio(double ratio);
    void setCurrentOffset(double offset);
    void calculateMultiplier();
    void warmup();

  private:

    current_c _currentCallback;
    unsigned char _adcBits;
    unsigned int _adcCounts;
    double _referenceVoltage;
    double _currentRatio;
    double _currentMidPoint;
    double _currentFactor;
    double _multiplier;


};

#endif
