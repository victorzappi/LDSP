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

// This code example uses this sounds from freesound:
// "323623__shpira__tech-drums.wav" by shpira ( https://freesound.org/s/323623/ ) licensed under CCBYNC 4.0

#include "LDSP.h"

// two set of parameters for left and right channel
float delayTime_sec[2] = {0.2, 0.3};
float feedback[2] = {0.6, 0.5};

//-------------------------------------------

// declare variables for circular buffers, one for left out channel, the other for right out channel
std::vector<float> delayBuffer[2];
int writePointer[2] = {0};
int readPointer[2] = {0};




bool setup(LDSPcontext *context, void *userData)
{
	if(context->audioOutChannels > 2)
	{
    	printf("This example can run with two audio output channels only and %d were detected /:\n", context->audioOutChannels);
    	return false;
	}


	// allocate the circular buffers to contain enough samples to cover 2 seconds
    delayBuffer[0].resize(2 * context->audioSampleRate);
	delayBuffer[1].resize(2 * context->audioSampleRate);
    
	// express delay time in samples and set up delay
	int delayTime_sampl_l = delayTime_sec[0] * context->audioSampleRate;
	readPointer[0] = ( writePointer[0] - delayTime_sampl_l + delayBuffer[0].size() ) % delayBuffer[0].size(); // modulo is good practice
	int delayTime_sampl_r = delayTime_sec[1] * context->audioSampleRate;
	readPointer[1] = ( writePointer[1] - delayTime_sampl_r + delayBuffer[1].size() ) % delayBuffer[0].size(); // modulo is good practice

    return true;
}

void render(LDSPcontext *context, void *userData)
{
	float dry = 0;
	float wet[2] = {0}; // left and right

	for(int n=0; n<context->audioFrames; n++)
	{
		dry = audioRead(context, n, 0); // mono input

		for(int chn=0; chn<context->audioOutChannels; chn++)
		{
			// get delayed signal
			wet[chn] = delayBuffer[chn][readPointer[chn]]; 
			
			// write to the circular buffer both input and feedback
			delayBuffer[chn][writePointer[chn]] = dry + feedback[chn] * wet[chn]; // overwrite the buffer at the write pointer

        	// increment and wrap both pointers
			writePointer[chn]++;
			if(writePointer[chn] >= delayBuffer[chn].size())
				writePointer[chn] = 0;
			readPointer[chn]++;
			if(readPointer[chn] >= delayBuffer[chn].size())
				readPointer[chn] = 0;

			float out = dry*0.5+wet[chn]*0.5; // mix dry and wet

            audioWrite(context, n, chn, out);
		}
	}
}

void cleanup(LDSPcontext *context, void *userData)
{

}