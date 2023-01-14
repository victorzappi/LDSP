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
    int verbose;
    // these have priority over deviceOut/InNum and if not specificied are automatically populated according to device numbers
    string deviceOutId;
    string deviceInId;
};

enum sensorState {
    sensor_present = 1,
    sensor_not_present = 0,
    sensor_not_supported = -1
};

enum ctrlOutState {
    device_configured = 1,
    device_not_configured = 0
};

enum digitalOuput {
    on = 1,
    off = 0
};

struct LDSPcontext {
	const float * const audioIn;
	float * const audioOut;
	const uint32_t audioFrames;
	const uint32_t audioInChannels;
	const uint32_t audioOutChannels;
	const float audioSampleRate;
    const float * const analogIn;
    float * const analogOut;
    const uint32_t analogInChannels;
    const uint32_t analogOutChannels;
    const sensorState *analogInSensorState;
    const ctrlOutState *analogCtrlOutputState;
    const string *analogInSensorDetails;
    const string *analogCtrlOutputDetails;
    const float *analogInNormalFactor;
    const float analogSampleRate;
	//uint64_t audioFramesElapsed;
};

//TODO change from analog to sensorIn
enum analogInChannel {
    chn_ain_accelX,
    chn_ain_accelY,
    chn_ain_accelZ,
    chn_ain_magX,
    chn_ain_magY,
    chn_ain_magZ,
    chn_ain_gyroX,
    chn_ain_gyroY,
    chn_ain_gyroZ,
    chn_ain_light,
    chn_ain_proximity,
    chn_ain_count
};

//TODO change from analog to ctrlOut
enum analogOutChannel {
    chn_aout_flashlight, 
    chn_aout_lcdBacklight,
    chn_aout_led,
    chn_aout_ledR,
    chn_aout_ledG,
    chn_aout_ledB, 
    chn_aout_buttonsBacklight,
    chn_aout_vibration,
    chn_aout_count
};


LDSPinitSettings* LDSP_InitSettings_alloc();

void LDSP_InitSettings_free(LDSPinitSettings* settings);

void LDSP_defaultSettings(LDSPinitSettings *settings);

int LDSP_initAudio(LDSPinitSettings *settings, void *userData);

void LDSP_initSensors(LDSPinitSettings *settings);

int LDSP_startAudio();

void LDSP_cleanupSensors();

void LDSP_cleanupAudio();

void LDSP_requestStop();


bool setup(LDSPcontext *context, void *userData);
void render(LDSPcontext *context, void *userData);
void cleanup(LDSPcontext *context, void *userData);


static inline void audioWrite(LDSPcontext *context, int frame, int channel, float value);
static inline float audioRead(LDSPcontext *context, int frame, int channel);

//TODO change to sensorRead() and ctrlWrite()
static inline float analogRead(LDSPcontext *context, analogInChannel channel);
static inline void analogWrite(LDSPcontext *context, analogOutChannel channel, float value);
static inline float analogRead(LDSPcontext *context, int frame, analogInChannel channel);
static inline void analogWrite(LDSPcontext *context, int frame, analogOutChannel channel, float value);
static inline void analogWriteOnce(LDSPcontext *context, int frame, analogOutChannel channel, float value);


static inline sensorState analogInSensorState(LDSPcontext *context, analogInChannel channel);
static inline string analogInSensorDetails(LDSPcontext *context, analogInChannel channel);
static inline float analogInNormFactor(LDSPcontext *context, analogInChannel channel);

//TODO analogCtrlOutputState(...)
//TODO analogCtrlOutputDetails(...)


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

// analogRead()
//
// Returns the value of the given analog input/sensor 
static inline float analogRead(LDSPcontext *context, analogInChannel channel) 
{
	    return context->analogIn[channel]; // some of these sensors may not be present or unsupported, they return 0
}
// for full compatibility with Bela, ignores frame
static inline float analogRead(LDSPcontext *context, int frame, analogInChannel channel)
{
    return analogRead(context, channel); // returns type of sensor even if not present on phone; returns "Not supported" if not supported by API
}

// analogInSensorState()
//
// returns sate of the sensor accessed on the given channel
static inline sensorState analogInSensorState(LDSPcontext *context, analogInChannel channel)
{
 
    return context->analogInSensorState[channel]; // sensor present or not present on phone, or unsupported by API
}

// analogInSensorDetails()
//
// returns sate of the sensor accessed on the given channel
static inline string analogInSensorDetails(LDSPcontext *context, analogInChannel channel)
{
    return context->analogInSensorDetails[channel]; // returns type of sensor even if not present on phone; returns "Not supported" if not supported by API
}

// analogInNormFactor()
//
// returns normalization factor of the given channel
// all channel inputs are normalized [-1,1] and clamped to the maximum de-normalized absolute value
static inline float analogInNormFactor(LDSPcontext *context, analogInChannel channel)
{
    return context->analogInNormalFactor[channel]; // returns 1 if sensor is not normalized, not present on phone or not supported by API
}


// analogWrite()
//
// Sets a given analog output channel to a value, that is always persistent
static inline void analogWrite(LDSPcontext *context, analogOutChannel channel, float value) 
{
    context->analogOut[channel] = value;
}
//TODO remove?
// for full compatibility with Bela, ignores frame
static inline void analogWrite(LDSPcontext *context, int frame, analogOutChannel channel, float value) 
{
    analogWrite(context, channel, value);
}
// for full compatibility with Bela, ignores frame
static inline void analogWriteOnce(LDSPcontext *context, int frame, analogOutChannel channel, float value) 
{
    analogWrite(context, channel, value);
}

#endif /* LDSP_H_ */
