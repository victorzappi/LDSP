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
#ifndef OUT_DEVICES_H_
#define OUT_DEVICES_H_

#include "LDSP.h"
#include "tinyalsaAudio.h" // for LDSPinternalContext
#include "hwConfig.h"
#include "enums.h"

#include <iostream>
#include <fstream>

static const string LDSP_analog_outDevices[chn_aout_count] = {
    "flashlight",
    "lcd-backlight",
    "led", 
    "buttons-backlight",
};

struct outdev_struct {
    bool configured;
    ofstream file;
    unsigned int maxVal;
    unsigned int prevVal;
    unsigned int initialVal;
};

//TODO next add digital! 
struct LDSPoutDevContext {
    outdev_struct analogOutDevices[chn_aout_count];
    float analogOutDevBuffer[chn_aout_count];
    outDeviceState analogDevicesStates[chn_aout_count];
    string analogDevicesDetails[chn_aout_count];
};


int LDSP_initOutDevices(LDSPinitSettings *settings, LDSPhwConfig *hwconfig);
void LDSP_cleanupOutDevices();

void outputDevWrite();
void write(ofstream *file, int value);

//---------------------------------------------------------------

inline void write(ofstream *file, int value)
{
    // write
    *file << to_string(value);
    file->flush();
}

#endif /* OUT_DEVICES_H_ */
