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
#include <math.h> // sinf
#include <unistd.h> // ulseep


float freqOsc = 440.0;
float freqLFO = 2;
float amplitude = 0.2;

//------------------------------------------------
float phaseOsc;
float phaseLFO;
float inverseSampleRate;

float phaseLFO_prev = 0.0;

bool setup(LDSPcontext *context, void *userData)
{
	if( !context->ctrlOutputsSupported[chn_cout_lcdBacklight] ||
		!context->ctrlOutputsSupported[chn_cout_vibration] )
	{
		printf("Both LCD backligt and vibration has to be supported to run this example /:\n");
		return false;
	}



    inverseSampleRate = 1.0 / context->audioSampleRate;
	phaseOsc = 0.0;
	phaseLFO = 0.0;

	// if we can set and get screen state reliably
	if(context->screenGetStateSupported)
	{
		// set screen on and set brigthness to starting lfo value
		bool screenOn = true;
		float brightness = 0.5;
		bool stayOn = true;
		screenSetState(screenOn, brightness, stayOn);

		// screenSetState() takes a few hundred millisec, wait patiently...
		while(!screenGetState())
			usleep(10000);
	}

    return true;
}

void render(LDSPcontext *context, void *userData)
{
	float osc, lfo;
	for(int n=0; n<context->audioFrames; n++)
	{
		// lfo oscillates between 0 and 1 and starts from 0.5
		lfo = map( sinf(phaseLFO), -1.0, 1.0, 0.0, 1.0 );
		phaseLFO += 2.0f * (float)M_PI * freqLFO * inverseSampleRate;
		while(phaseLFO > 2.0f *M_PI)
			phaseLFO -= 2.0f * (float)M_PI;
		
		osc = sinf(phaseOsc);
		phaseOsc += 2.0f * (float)M_PI * freqOsc * inverseSampleRate;
		while(phaseOsc > 2.0f *M_PI)
			phaseOsc -= 2.0f * (float)M_PI;
		
		for(int chn=0; chn<context->audioOutChannels; chn++)
            audioWrite(context, n, chn, amplitude*osc*lfo);
	}

	// update backlight in sync with LFO [notice that this is updated onces per render call]
	ctrlOutputWrite(context, chn_cout_lcdBacklight, lfo);

	// turn vibration on for the first semicycle of the LFO
	if(phaseLFO_prev >= phaseLFO) // the cycle starts when the phase wraps up
		ctrlOutputWrite(context, chn_cout_vibration, 500.0/freqLFO);
	phaseLFO_prev = phaseLFO;
}

void cleanup(LDSPcontext *context, void *userData)
{

}