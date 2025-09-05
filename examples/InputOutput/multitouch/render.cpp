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
#include "wavetable.h"

#define WAVETABLE_SIZE 512
#define MAX_TOUCHES 3


float freq[MAX_TOUCHES] = {261.63, 336.82, 408.82}; // Wendy Carlos' Alpha tuning major triad for C4
float ampMax = 0.2;

//-------------------------------------------------

Wavetable wavetable[MAX_TOUCHES];

bool touchPrev[MAX_TOUCHES] = {false, false, false};

float amp[MAX_TOUCHES] = {0, 0, 0};



bool setup(LDSPcontext *context, void *userData)
{

	int slots = context->mtInfo->touchSlots;
	if(slots < MAX_TOUCHES)
		printf("This phone supports %d touches only, hence only %d of the %d voices of this example can be controlled\n", slots, slots, MAX_TOUCHES);


	std::vector<float> wave;

    wave.resize(WAVETABLE_SIZE);
	// fill vector with single sinusoidal period
	for(unsigned int n = 0; n < wave.size(); n++) 
		wave[n] = sinf(2.0 * M_PI * (float)n / (float)wave.size());
	

	// pass vector to wavetable
	for(int i=0; i<MAX_TOUCHES; i++) 
	{
		wavetable[i].setup(context->audioSampleRate, wave, true);
		// set cycle frequency
		wavetable[i].setFrequency(freq[i]);
	}
	
	ampMax = ampMax/MAX_TOUCHES;

    return true;
}

void render(LDSPcontext *context, void *userData)
{

	for(int i=0; i<MAX_TOUCHES; i++) 
	{
		bool touch = (multiTouchRead(context, chn_mt_id, i) != -1);
		// new touch
		if(touch && !touchPrev[i])
			amp[i] = ampMax; // activate output
		// touch released
		else if(!touch && touchPrev[i]) 
			amp[i] = 0; // silence output

		touchPrev[i] = touch;
	}

	for(int n=0; n<context->audioFrames; n++)
	{
		float out = 0;
		for(int i=0; i<MAX_TOUCHES; i++) 
			out += amp[i]*wavetable[i].process();
		
		for(int chn=0; chn<context->audioOutChannels; chn++)
            audioWrite(context, n, chn, out);
	}
}

void cleanup(LDSPcontext *context, void *userData)
{

}
