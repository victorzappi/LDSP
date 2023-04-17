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
#ifndef CTRL_INPUTS_H_
#define CTRL_INPUTS_H_

#include "LDSP.h"

#include <linux/input.h>
#include <vector>
#include <unordered_map> // unordered_map
#include <atomic>
#include <memory>

#define QUEUE_MAX_SIZE 3

using std::vector;
using std::unordered_map;
using std::atomic;
using std::shared_ptr;


const int chn_cin_count = chn_btn_count+chn_mt_count;

// this are just the names we are interested into
// BE CAREFUL! this has to be updated manually, according to:
// btnInputChannel enum in LDSP.h [order]
// AND
// multiTouchInputChannel enum in LDSP.h [order]
// note that we have an extra entry here [touch_present], that is not associated to any channel in LDSP.h
// it is associated with a dedicated function though!
static const string LDSP_ctrlInput[chn_cin_count] = { 
    "power-button",         // up/down
    "volume-up-button",     // up/down
    "volume-down-button",   // up/down
    "touch-any",            // at least one touch on screen
    "touch-x-pos",          // touch's x, can be normalized via screenWidth
    "touch-y-pos",          // touch's y, can be normalized via screenHeight
    "touch-major-axis",     // major axis of touch ellipse, can be normalized via touchAxisMax
    "touch-minor-axis",     // minor axis of touch ellipse, can be normalized via touchAxisMax
    "touch-orientation",    // orientation of touch ellipse, normalized over 0,2pi
    "touch-x-hover-pos",    // x pos of center of finger/tool detected above touch, can be normalized via screenWidth
    "touch-y-hover-pos",    // y pos of center of finger/tool detected above touch, can be normalized via screenHeight
    "touch-major-width",    // major axis of hovering finger/tool ellipse, can be normalized via touchWidthMax
    "touch-minor-width",    // minor axis of hovering finger/tool ellipse, can be normalized via touchWidthMax
    "touch-pressure",       // ratio between touches_majorAxis / touches_majorWidth 
    "touch-id"              // to keep track of motion
};


struct ctrlInput_struct {
    bool supported;
    bool isMultiInput;
    vector< shared_ptr< atomic<signed int> > > value; // vector of atomic containers, for thread-safety
    // we have to use pointers though, because vector cannot deal with atomics directly
}; 

// number of elements in vectors depends on how many slots phone supports [touchSlots]
struct LDSPctrlInputsContext {
    unsigned int inputsCount;
    ctrlInput_struct ctrlInputs[chn_cin_count];
    multiTouchInfo mtInfo;
    unordered_map<unsigned short, unordered_map<int, int> > ctrlInputsEvent_channel;
    // BE CAREFUL, mapping is manual!
    LDSPctrlInputsContext() : ctrlInputsEvent_channel({
        // single ctrl inputs [with queue]
        { EV_KEY, { {KEY_POWER, chn_btn_power}, 
                {KEY_VOLUMEUP, chn_btn_volUp}, 
                {KEY_VOLUMEDOWN, chn_btn_volDown},
                {BTN_TOUCH, chn_btn_count+chn_mt_anyTouch} } 
        },
        // multi ctrl inputs [with vector queue]
        { EV_ABS, { {ABS_MT_POSITION_X, chn_btn_count+chn_mt_x}, 
                    {ABS_MT_POSITION_Y, chn_btn_count+chn_mt_y},
                    {ABS_MT_TOUCH_MAJOR, chn_btn_count+chn_mt_majAxis},
                    {ABS_MT_TOUCH_MINOR, chn_btn_count+chn_mt_minAxis},
                    {ABS_MT_ORIENTATION, chn_btn_count+chn_mt_orientation},
                    {ABS_MT_TOOL_X, chn_btn_count+chn_mt_hoverX},
                    {ABS_MT_TOOL_Y, chn_btn_count+chn_mt_hoverX},
                    {ABS_MT_WIDTH_MAJOR, chn_btn_count+chn_mt_majWidth},
                    {ABS_MT_WIDTH_MINOR, chn_btn_count+chn_mt_minWidth},
                    {ABS_MT_PRESSURE, chn_btn_count+chn_mt_pressure},
                    {ABS_MT_TRACKING_ID, chn_btn_count+chn_mt_id} }
        }
    }) {}
    int *ctrlInBuffer;
    ctrlInState *ctrlInStates;
    string *ctrlInDetails;
};
// Linux multi-touch protocol explained here: https://www.kernel.org/doc/html/latest/input/multi-touch-protocol.html

void readCtrlInputs();


#endif /* CTRL_INPUTS_H_ */
