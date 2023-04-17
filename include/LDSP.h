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
#ifndef LDSP_H_
#define LDSP_H_

#include <string>
#include "BelaUtilities.h"
#include "hwConfig.h"

#define MAX_TOUCHES 20

using std::string;

struct LDSPinitSettings {
	// these items might be adjusted by the user:
    int card;
    int deviceOutNum;
    int deviceInNum;
	int periodSize;
	int periodCount;
    int numAudioOutChannels;
    int numAudioInChannels;
    float samplerate;
    string pcmFormatString;
    string pathOut;
    string pathIn;
    int outputOnly;
    int sensorsOff;
    int ctrlInputsOff;
    int ctrlOutputsOff;
    int perfModeOff;
    int verbose;
    // these have priority over deviceOut/InNum and if not specificied are automatically populated according to device numbers
    string deviceOutId;
    string deviceInId;
    string projectName;
};

enum sensorState {
    sensor_present = 1,
    sensor_not_present = 0,
    sensor_not_supported = -1,
};

enum ctrlInState {
    ctrlInput_supported = 1,
    ctrlInput_not_supported = 0
};


enum ctrlOutState {
    ctrlOutput_configured = 1,
    ctrlOutput_not_configured = 0
};

enum digitalOuput {
    on = 1,
    off = 0
};

struct multiTouchInfo {
    float screenResolution[2];
    int touchSlots;
    int touchAxisMax;
    int touchWidthMax;
};

enum screenState {
    screenOn = 1,
    screenOff = 0
};

struct LDSPcontext {
	const float * const audioIn;
	float * const audioOut;
	const uint32_t audioFrames;
	const uint32_t audioInChannels;
	const uint32_t audioOutChannels;
	const float audioSampleRate;
    const float * const sensors;
    const int * const ctrlInputs;
    float * const ctrlOutputs;
    const uint32_t sensorChannels;
    const uint32_t ctrlInChannels;
    const uint32_t ctrlOutChannels;
    const sensorState * const sensorsState;
    const ctrlInState * const ctrlInputsState;
    const ctrlOutState * const ctrlOutputsState;
    const string * const sensorsDetails;
    const string * const ctrlInputsDetails;
    const string * const ctrlOutputsDetails;
    //const float *analogInNormalFactor;
    const float controlSampleRate;
    const multiTouchInfo * const mtInfo;
	//uint64_t audioFramesElapsed;
};

enum sensorChannel {
    chn_sens_accelX,
    chn_sens_accelY,
    chn_sens_accelZ,
    chn_sens_magX,
    chn_sens_magY,
    chn_sens_magZ,
    chn_sens_gyroX,
    chn_sens_gyroY,
    chn_sens_gyroZ,
    chn_sens_light,
    chn_sens_proximity,
    chn_sens_count
};

enum ctrlOutputChannel {
    chn_cout_flashlight, 
    chn_cout_lcdBacklight,
    chn_cout_led,
    chn_cout_ledR,
    chn_cout_ledG,
    chn_cout_ledB, 
    chn_cout_buttonsBacklight,
    chn_cout_vibration,
    chn_cout_count
};

enum btnInputChannel {
    chn_btn_power,
    chn_btn_volUp,
    chn_btn_volDown,
    chn_btn_count
};

enum multiTouchInputChannel {
    chn_mt_anyTouch,
    chn_mt_x,
    chn_mt_y,
    chn_mt_majAxis,
    chn_mt_minAxis,
    chn_mt_orientation,
    chn_mt_hoverX,
    chn_mt_hoverY,
    chn_mt_majWidth,
    chn_mt_minWidth,
    chn_mt_pressure,
    chn_mt_id,
    chn_mt_count
};



LDSPinitSettings* LDSP_InitSettings_alloc();

void LDSP_InitSettings_free(LDSPinitSettings* settings);

void LDSP_defaultSettings(LDSPinitSettings *settings);

LDSPhwConfig* LDSP_HwConfig_alloc();

void LDSP_HwConfig_free(LDSPhwConfig* hwconfig);

int LDSP_parseHwConfigFile(LDSPinitSettings *settings, LDSPhwConfig *hwconfig);

int LDSP_initAudio(LDSPinitSettings *settings, void *userData);

void LDSP_initSensors(LDSPinitSettings *settings);

void LDSP_initCtrlInputs(LDSPinitSettings *settings);

int LDSP_initCtrlOutputs(LDSPinitSettings *settings, LDSPhwConfig *hwconfig);

int LDSP_startAudio(void *userData);

void LDSP_cleanupSensors();

void LDSP_cleanupCtrlInputs();

void LDSP_cleanupCtrlOutputs();

void LDSP_cleanupAudio();

void LDSP_requestStop();


bool setup(LDSPcontext *context, void *userData);
void render(LDSPcontext *context, void *userData);
void cleanup(LDSPcontext *context, void *userData);


static inline void audioWrite(LDSPcontext *context, int frame, int channel, float value);
static inline float audioRead(LDSPcontext *context, int frame, int channel);

static inline float sensorRead(LDSPcontext *context, sensorChannel channel);
static inline int buttonRead(LDSPcontext *context, btnInputChannel channel);
static inline int multitouchRead(LDSPcontext *context, multiTouchInputChannel channel, int touchSlot=0);
static inline void ctrlOutputWrite(LDSPcontext *context, ctrlOutputChannel channel, float value);

static inline sensorState sensorsState(LDSPcontext *context, sensorChannel channel);
static inline string sensorsDetails(LDSPcontext *context, sensorChannel channel);
//static inline float analogInNormFactor(LDSPcontext *context, sensorChannel channel);

//TODO ctrlOutputs/InputsState(...)
//TODO ctrlOutputs/InputsDetails(...)

void screenSetState(bool stateOn, float brightness=1, bool keepOn=false);
bool screenGetState(); // this is a heavy function, use with caution

//-----------------------------------------------------------------------------------------------
// inline

// audioRead()
//
// Returns the value of the given audio input at the given frame number
static inline float audioRead(LDSPcontext *context, int frame, int channel) 
{
	return context->audioIn[frame * context->audioInChannels + channel];
}

// audioWrite()
//
// Sets a given audio output channel to a value for the current frame
static inline void audioWrite(LDSPcontext *context, int frame, int channel, float value) 
{
	context->audioOut[frame * context->audioOutChannels + channel] = value;
}

// sensorRead()
//
// Returns the value of the given analog input/sensor 
static inline float sensorRead(LDSPcontext *context, sensorChannel channel) 
{
	    return context->sensors[channel]; // some of these sensors may not be present or unsupported, they return 0
}

// sensorsState()
//
// returns sate of the sensor accessed on the given channel
static inline sensorState sensorsState(LDSPcontext *context, sensorChannel channel)
{
 
    return context->sensorsState[channel]; // sensor present or not present on phone, or unsupported by API
}

// sensorsDetails()
//
// returns sate of the sensor accessed on the given channel
static inline string sensorsDetails(LDSPcontext *context, sensorChannel channel)
{
    return context->sensorsDetails[channel]; // returns type of sensor even if not present on phone; returns "Not supported" if not supported by API
}

// analogInNormFactor()
//
// returns normalization factor of the given channel
// all channel inputs are normalized [-1,1] and clamped to the maximum de-normalized absolute value
// static inline float analogInNormFactor(LDSPcontext *context, sensorChannel channel)
// {
//     return context->analogInNormalFactor[channel]; // returns 1 if sensor is not normalized, not present on phone or not supported by API
// }


// ctrlOutputWrite()
//
// Sets a given ctrl output channel to a value, that is always persistent [except for vibration]
static inline void ctrlOutputWrite(LDSPcontext *context, ctrlOutputChannel channel, float value) 
{
    context->ctrlOutputs[channel] = value;
}

static inline int buttonRead(LDSPcontext *context, btnInputChannel channel)
{
    return context->ctrlInputs[channel];
}

static inline int multitouchRead(LDSPcontext *context, multiTouchInputChannel channel, int touchSlot)
{
    if(channel==chn_mt_anyTouch)
        return context->ctrlInputs[chn_btn_count+chn_mt_anyTouch];
    else
        return context->ctrlInputs[chn_btn_count+1+(channel-1)*context->mtInfo->touchSlots + touchSlot];
}


#endif /* LDSP_H_ */
