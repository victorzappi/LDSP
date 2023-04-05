/*
 * [2-Clause BSD License]
 *
 * Copyright 2022 Victor Zappi
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef SENSORS_H_
#define SENSORS_H_


//VIC currently, we are leveraging the HAL to access the sensors of the phone
// in particular, we activate sensors and pop readings using the android/sensor.h API, which in turn calls some low level vendor libs
// this happens in multi-threading, managed by the OS
// it seems that the sensor HAL was introduced with Android 4.1 Jelly Bean, API level 16
// and the list of available sensors on Android phones has grown over the years
// we are sure that the original list of sensors is compatible with all Android versions and with all phones [worst case scenario they are not present on the phone]
// hence, Android 4.1 Jelly Bean, API level 16 is the oldest that we can support right now sensor-wise

#include <android/sensor.h>
#include <unordered_map> // unordered_map
#include "LDSP.h"
#include "tinyalsaAudio.h" // for LDSPinternalContext
#include "enums.h"

using std::unordered_map;

// wrapper of android sensor types, to ensure compatibility with older and newer versions of Android
// note that we include only a subset of all the sensors supported by android, because many of them are not very 'useful'
// this is a special wrapper, that permits to print the ENUM ad to cycle via indices
ENUM(LDSP_sensor, short,
    accelerometer = ASENSOR_TYPE_ACCELEROMETER,
    magnetometer  = ASENSOR_TYPE_MAGNETIC_FIELD,
    gyroscope     = ASENSOR_TYPE_GYROSCOPE,
    light         = ASENSOR_TYPE_LIGHT,
    proximity     = ASENSOR_TYPE_PROXIMITY,
    count         = 5 // BE CAREFUL! this entry has to be updated manually and it does not matter if has same value as other entries
    // minimum set of sensors, compatible all the way down to Android 4.1 Jelly Bean, API level 16
    //TODO extend this according to API LEVEL
)
// number of input channels associated with each sensor
// BE CAREFUL! this has to be updated manually, according to:
// LDSP_sensor ENUMs [same number of entries, sam order]
// AND
// sensorChannel enum in LDSP.h [each LDSP_sensor entry has to correspond to 1 or mulitple sensorChannel enum entry, according to number of channels set in here]
static const string sensors_channelsInfo[LDSP_sensor::count][2] = {
    {"3", "acceleration on x [m/s^2], acceleration on y [m/s^2], acceleration on z [m/s^2]"}, // accelerometer 
    {"3", "magentic field on x [uT], magentic field on y [uT], magentic field on z [uT]"}, // magnetometer
    {"3", "rate of rotation around x [rad/s], rate of rotation around y [rad/s], rate of rotation around z [rad/s]"}, // gyroscope
    {"1", "illuminance [lx]"}, // light
    {"1", "distance [cm]"}  // proximity 
}; 
//VIC any ways to retrieve number of channels per sensor from freaking android API?!?!?

// if max values are reported in here, the sensor input is normalized
// BE CAREFUL! this has to updated manually, according to LDSP_sensor ENUMs
// static const float sensors_max[LDSP_sensor::count] = {
//     2*9.8,  // 2g [m/s^2]
//     1000,   // 1000 uT      
//     -1,     // no max, no normalization
//     -1,
//     10
// }; 


struct sensor_struct {
    const ASensor *asensor;
    bool present;
    unsigned int type;
    unsigned int numOfChannels;
    sensorChannel *channels;
};

struct LDSPsensorsContext {
    unsigned int sensorsCount = 0;
    sensor_struct sensors[LDSP_sensor::count];
    unordered_map<int, int> sensorsType_index; // automatically populated
    float *sensorBuffer;
    sensorState *sensorsStates;
    string *sensorsDetails;
};

void readSensors();


//TODO add user function that can spawn a low prio thread that unlocks screen and keeps it unlocked
// otherwise on some phones sensors do not work [idle] ---> should be called in projects only if needed!
// can do via shell with: keyinput event 26

#endif /* SENSORS_H_ */

