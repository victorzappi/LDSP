/*
 * [2-Clause BSD License]
 *
 * Copyright 2017 Victor Zappi
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

/*
 * priority_utils.h
 *
 *  Created on: 2015-10-30
 *      Author: Victor Zappi
 */

#ifndef PRIORITY_UTILS_H_
#define PRIORITY_UTILS_H_

#include <pthread.h>

// main threads ordered by priority [order 0 is max priority]
constexpr unsigned int LDSPprioOrder_audio = 0;
constexpr unsigned int LDSPprioOrder_ctrlInputs = 1;
// ctrlOutputs run on the audio thread and are immediate
// sensor data are retrieved from the audio thread, but they are read in an Android sever thread we have no control over

// these are for optional threads, that are spawn only if the associated features are enabled
constexpr unsigned int LDSPprioOrder_screenCtl = 50;

constexpr unsigned int LDSPprioOrder_midiRead = 10;
constexpr unsigned int LDSPprioOrder_midiWrite = 10;

constexpr unsigned int LDSPprioOrder_oscReceive = 2;
constexpr unsigned int LDSPprioOrder_oscSend = 2;

constexpr unsigned int LDSPprioOrder_arduinoRead = 2;
constexpr unsigned int LDSPprioOrder_arduinoWrite = 2;

// for either web socket server [wsserver] and web server [wbserver]
constexpr unsigned int LDSPprioOrder_wserverServe = 20;
constexpr unsigned int LDSPprioOrder_wserverClient = 20;


//-----------------------------------------------------------------------------------------------------------
// set maximum priority to this thread
//-----------------------------------------------------------------------------------------------------------
void set_priority(int order, bool verbose); // 0 is max prio, cos at front end we do not know what's the highest prio
void set_niceness(int niceness, bool verbose); // -20 is highest prio niceness, it's a known standard



#endif /* PRIORITY_UTILS_H_ */
