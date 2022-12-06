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

#include "LDSP.h"
//#include "libraries/LinInterpolator.h"
#include "libraries/Bela/Biquad/Biquad.h"
#include <math.h> // sin

// #include <iostream>
// #include <fstream>
// std::ofstream torch_file;
// std::ifstream max_file;


float duration = -1; // [s]
float centralFrequency = 440.0;
float badnwidth = centralFrequency/2.0;
float maxAmplitude = 0.3;
float amplitudeDelta = 0.9;

float lcdLightFreq = 1;
float lcdLightPhasePrev = -1;
unsigned int flashlight = 0;

//------------------------------------------------

int periodsToPlay;
float phase;
float inverseSampleRate;

float lcdLightPhase;
float lcdLightInvSrate;

//LinInterpolator accXinterpolated;
//float prev_acc_x;

Biquad *smoothingFilter;

bool setup(LDSPcontext *context, void *userData)
{
    inverseSampleRate = 1.0 / context->audioSampleRate;
	phase = 0.0;

	periodsToPlay = duration * context->audioSampleRate / context->audioFrames;

	BiquadCoeff::Settings filterSettings;
	filterSettings.fs = context->audioSampleRate;
	filterSettings.type = BiquadCoeff::lowpass;
	filterSettings.cutoff = context->analogSampleRate/4;
	filterSettings.q = 0.707;
	filterSettings.peakGainDb = 0;

	smoothingFilter = new Biquad(filterSettings);


	//-----------------------------------------------------------


    lcdLightInvSrate = 1.0 / context->analogSampleRate;
	lcdLightPhase = 0.0;

	
	
	digitalWrite(context, chn_dout_ledB, on);
	

    return true;
}

void render(LDSPcontext *context, void *userData)
{
	float acc_x = analogRead(context, chn_ain_accelX);
	float acc_z = analogRead(context, chn_ain_accelZ);

	//accXinterpolated.setup(prev_acc_x, acc_x, context->audioFrames);
	//prev_acc_x = acc_x;

	float frequency = centralFrequency + badnwidth*acc_z;

	float out;
	for(int n=0; n<context->audioFrames; n++)
	{
		float amplitude = maxAmplitude*(1-amplitudeDelta);
		//amplitude += accXinterpolated[n]*amplitudeDelta;
		amplitude += smoothingFilter->process(acc_x)*amplitudeDelta;

		out = amplitude * sinf(phase);
		phase += 2.0f * (float)M_PI * frequency * inverseSampleRate;
		while(phase > 2.0f *M_PI)
			phase -= 2.0f * (float)M_PI;

        //out = audioRead(context, n, 0);
		
		for(int chn=0; chn<context->audioOutChannels; chn++)
            audioWrite(context, n, chn, out);

	    //printf("out %f\n", out);
	}

	float lcdLight = map(sinf(lcdLightPhase), -1, 1, 0.1, 1);
	lcdLightPhase += 2.0f * (float)M_PI * lcdLightFreq * lcdLightInvSrate;
	while(lcdLightPhase > M_PI)
		lcdLightPhase -= 2.0f * (float)M_PI;
	analogWrite(context, chn_aout_lcdBacklight, lcdLight);

	if(lcdLightPhasePrev < 0 && lcdLightPhase >=0)
	{
		digitalWrite(context, chn_dout_vibration, 500);
		flashlight = (1-flashlight);
		digitalWrite(context, chn_dout_flashlight, flashlight);
	}
	lcdLightPhasePrev = lcdLightPhase;


	if(--periodsToPlay==0)
         LDSP_requestStop();
}

void cleanup(LDSPcontext *context, void *userData)
{
    delete smoothingFilter;
}