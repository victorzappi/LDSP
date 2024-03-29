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
#ifndef CTRL_OUTPUTS_H_
#define CTRL_OUTPUTS_H_

#include "LDSP.h"
#include "enums.h"

#include <iostream>
#include <fstream>
#include <array>

using std::ofstream;
using std::to_string;
using std::array;

// BE CAREFUL! this has to be updated manually, according to:
// ctrlOutputChannel enum in LDSP.h [order]
// and
// the keys in the hw_config.json file [names]
static const string LDSP_ctrlOutput[chn_cout_count] = {
    "flashlight",
    "lcd-backlight",
    "led",
    "led-red",
    "led-green",
    "led-blue", 
    "buttons-backlight",
    "vibration" // vibration is time-based and what we pass is the duration of the vibration [ms]
};

// for autocompletion of hw config of ctrl outputs
struct ctrlOutputKeywords {
    array<array<const string, 2>, chn_cout_count> folders = {
        {
            {"leds", ""},
            {"leds", ""},
            {"leds", ""},
            {"leds", ""},
            {"leds", ""},
            {"leds", ""},
            {"leds", ""},
            {"timed_output", "leds"}
        }
    };
    array<array<const string, 3>, chn_cout_count> subFolders_strings = {
        {
            {"torch", "flash", ""},
            {"lcd", "", ""},
            {"white", ""/* "rgb" */, ""}, //VIC teporarily disabled rgb led, because not sure how it works and can generate crashes!
            {"red", "", ""},
            {"green", "", ""},
            {"blue", "", ""},
            {"buttons", "", ""},
            {"vibrator", "", ""}
        }
    };
    array<array<const string, 3>, chn_cout_count> files = {
        {
            {"brightness", "max_brightness", ""},
            {"brightness", "max_brightness", ""},
            {"brightness", "max_brightness", ""},
            {"brightness", "max_brightness", ""},
            {"brightness", "max_brightness", ""},
            {"brightness", "max_brightness", ""},
            {"brightness", "max_brightness", ""},
            {"enable", "activate", "duration"}
        }
    };
};

// to find property that discloses state of screen
// if everything goes well, a single combination of service and prop will be chosen in probeScreenCommands()
// equivalent to command line:
// dumpsys service[idx] | grep prop[idx]
struct screenCtrlsCommands {
    static const unsigned int idx_cnt = 6;
    string service[idx_cnt] = {"dumpsys power", "dumpsys power", "dumpsys power",           "dumpsys window",              "dumpsys display",  "dumpsys display"};
    string prop[idx_cnt]    = {"mScreenOn=",    "mWakefulness=", "Display Power: state=",   "mWindowManagerDrawComplete=", "mBlanked=",        "mScreenState="};
    string on[idx_cnt]      = {"true",          "true",          "ON",                      "true",                        "false",            "ON"};
    string off[idx_cnt]     = {"false",         "false",         "OFF",                     "false",                       "true",             "OFF"};
    int idx = -1;
};


struct ctrlout_struct {
    bool configured;
    ofstream file;
    unsigned int scaleVal;
    unsigned int prevVal;
    unsigned int initialVal;
};

struct LDSPctrlOutputsContext {
    ctrlout_struct ctrlOutputs[chn_cout_count];
    float ctrlOutBuffer[chn_cout_count];
    bool ctrlOutSupported[chn_cout_count];
};

void writeCtrlOutputs();
void write(ofstream *file, int value);

//---------------------------------------------------------------

inline void write(ofstream *file, int value)
{
    // write
    *file << to_string(value);
    file->flush();
}

#endif /* CTRL_OUTPUTS_H_ */
