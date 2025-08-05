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
#include <libraries/Scope/Scope.h>
#include <cmath> 
#include <ctime>

float amplitude = 0.2;
float centralFrequency = 440.0;
int octaveRange = 2;


//------------------------------------------------
float rootFrequency = centralFrequency;
float phase;
float inverseSampleRate;

int btnDwn_prev = 0;

// instantiate the scope
Scope scope;



bool setup(LDSPcontext *context, void *userData)
{

	if( !context->sensorsSupported[chn_sens_accelX] ||
		!context->sensorsSupported[chn_sens_accelY] )
	{
		printf("Accelerometer not present on this phone, the example cannot be run ):\n");
		return false;	
	}

    inverseSampleRate = 1.0 / context->audioSampleRate;
	phase = 0.0;

	// tell the scope how many channels and the sample rate
	scope.setup(3, context->audioSampleRate);

    return true;
}

void render(LDSPcontext *context, void *userData)
{
	// check for button press (volume down) and generate new root frequency
	int btnDwn = buttonRead(context, chn_btn_volDown);
	if(btnDwn==1 && btnDwn_prev==0)
	{
		int octaveShift = std::rand() % (2*octaveRange+1) - octaveRange;    // gives -octaveRange to +octaveRange	
		rootFrequency = centralFrequency * pow(2, octaveShift);
	}
	btnDwn_prev = btnDwn;

	// read accelerometer x channel
	float acc_x = sensorRead(context, chn_sens_accelX);
	// clip to -1 g and 1 g and normalize
	acc_x = constrain(acc_x, -9.8, 9.8) / 9.8;
	// map to a fith above and below root note
	float bent_freq = map(acc_x, -1.0, 1.0, rootFrequency * 2.0f / 3.0f, rootFrequency * 3.0f / 2.0f); // map x acceleration to osc frquency

	for(int n=0; n<context->audioFrames; n++)
	{
		float out = amplitude * sinf(phase);
		phase += 2.0f * (float)M_PI * bent_freq * inverseSampleRate;
		while(phase > 2.0f *M_PI)
			phase -= 2.0f * (float)M_PI;
		
		for(int chn=0; chn<context->audioOutChannels; chn++)
            audioWrite(context, n, chn, out);

		// log audio output, normalized acceleration and state of button (cannot be an int! and we scale it down for visibility)
		scope.log(out, acc_x, btnDwn*0.5);
	}
}

void cleanup(LDSPcontext *context, void *userData)
{

}