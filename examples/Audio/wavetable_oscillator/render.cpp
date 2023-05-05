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

float freq = 220; // Hz
float amp = 0.5;

//-------------------------------------------------

Wavetable wavetableOsc;


bool setup(LDSPcontext *context, void *userData)
{
	std::vector<float> wavetable;

    wavetable.resize(WAVETABLE_SIZE);
	// fill vector with single sinusoidal cycle
	for(unsigned int n = 0; n < wavetable.size(); n++) 
		wavetable[n] = sinf(2.0 * M_PI * (float)n / (float)wavetable.size());
	

	// pass vector to wavetable
	bool useInterpolation = true;
	wavetableOsc.setup(context->audioSampleRate, wavetable, useInterpolation);
	// set cycle frequency
    wavetableOsc.setFrequency(freq);
		

    return true;
}

void render(LDSPcontext *context, void *userData)
{
	for(int n=0; n<context->audioFrames; n++)
	{
		float out = amp*wavetableOsc.process();
		
		for(int chn=0; chn<context->audioOutChannels; chn++)
            audioWrite(context, n, chn, out);
	}
}

void cleanup(LDSPcontext *context, void *userData)
{

}