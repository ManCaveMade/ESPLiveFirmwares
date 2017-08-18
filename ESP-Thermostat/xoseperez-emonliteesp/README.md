# EmonLiteESP

Current measure library for ESP8266 based board. Based on the [EmonLib library][1] but not API compatible!

Features:

* Customizable ADC bit count
* Customizable ADC voltage reference
* Read ADC values via callbacks

## Usage

First you will have to **add the library** to your project and **instantiate an object** for it.

```
#include "EmonLiteESP.h"
EmonLiteESP power;
```

Then create a **callback function** that the object will use to retrieve the ADC value. This apprach gives more flexibility since you can use built in ADC GPIO or an I2C ADC chip module like the ADS1115.

```
unsigned int currentCallback() {
    return analogRead(0);
}
```

No you can **configure** the object in your setup method:

```
power.initCurrent(currentCallback, ADC_BITS, REFERENCE_VOLTAGE, CURRENT_RATIO);
```

Here the different parameters are:

* **ADC_BITS**: precision of the ADC. Arduinos and ESP8266 use 10bits ADCs, but the ADS1115 is a 16bits ADC.
* **REFERENCE_VOLTAGE**: The voltage the full range refers to. For a bare ESP8266 chip this is 1.0V, if using NodeMCU or HUZZAH they have a voltage divider so the reference voltage is 3.3V.
* **CURRENT_RATIO**: This is the value in amps for a 1V output. Some current sensors like the YHDC SCT-013-030 have it written in the enclosure: 30A 1V. If you are using a current sensor that outputs a current (no built in burden resistor) it will depend on the turns ratio between the primary and secondary coils in the sensor and the burden resistor you use. Check about this constant in the post about [calibration][2] in the Open Energy Monitor site.

The library will calculate the proper precision output based on the calculated resolution.

To get the **current reading** (in amps) at any time you just have to call the ```getCurrent``` method passing the number of samples to take and average using root-mean-square calculation.

```
double current = power.getCurrent(NUMBER_OF_SAMPLES);
int power = current * MAINS_VOLTAGE;
```

Check the ```basic.cpp``` example for a functional code. You can also find examples using ADS1115 4 channels ADC or ADC121 one channel ADC.


[1]: https://github.com/openenergymonitor/EmonLib
[2]: https://openenergymonitor.org/emon/buildingblocks/calibration
