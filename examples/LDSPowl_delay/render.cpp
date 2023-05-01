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
#include "libraries/Biquad/Biquad.h"
#include <vector>
#include <math.h>

float max_delay = 0.5; // [s]
// initial delay time
float delay = 0.3; // [s]

// declare variables for circular buffer
std::vector<float> gDelayBuffer;
int gWritePointer = 0;
int gReadPointer = 0;

const float G=9.8;

// float acc_x_max = G;
// float acc_x_min = -G;

float acc_y_max = -3.7;
float acc_y_min = -8;

float fdbck_max = 0.98;
float fdbck_min = 0.0;

// float cutoff_max = 20e3;
// float cutoff_min = 40;

//------------------------------------------------

float inverseSampleRate;

Biquad *smoothingFilter; // acc_x
// Biquad *lowPass;

int delayInSamples;

// float map_max;
// float map_min;


bool setup(LDSPcontext *context, void *userData)
{
    inverseSampleRate = 1.0 / context->audioSampleRate;

	// smoothing filter for y-axis acceleromoter
	// signal oversampled at audio rate 
	BiquadCoeff::Settings filterSettings;
	filterSettings.fs = context->audioSampleRate;
	filterSettings.type = BiquadCoeff::lowpass;
	filterSettings.cutoff = context->controlSampleRate/4;
	filterSettings.q = 0.707;
	filterSettings.peakGainDb = 0;
	smoothingFilter = new Biquad(filterSettings);
	
	// audio low pass filter 
	// filterSettings.fs = context->audioSampleRate;
	// filterSettings.type = BiquadCoeff::lowpass;
	// filterSettings.cutoff = 20000;
	// filterSettings.q = 4;
	// filterSettings.peakGainDb = 0;
	// lowPass = new Biquad(filterSettings);

	// allocate the circular buffer to contain enough samples to cover ma delay in seconds
    gDelayBuffer.resize(max_delay * context->audioSampleRate);
    
	// express current delay in samples
	delayInSamples = delay * context->audioSampleRate;

	// delay defined by read pointer's distance from write pointer [in samples] within circular buffer
    gReadPointer = ( gWritePointer - delayInSamples + gDelayBuffer.size() ) % gDelayBuffer.size(); // modulo is good practice

	// for logarithmic rescaling of acc_y to cut-off mapping
	// map_max = log(cutoff_max-cutoff_min);
	// map_min = log(cutoff_min);

    return true;
}

void render(LDSPcontext *context, void *userData)
{
	float acc_y = sensorRead(context, chn_sens_accelY);

	// clip retrieved value
	acc_y = constrain(acc_y, acc_y_min, acc_y_max);
	
	// acc_y mapping to cut-off, with logarithmic rescaling
	// float map_cutoff = map(smoothingFilter[1]->process(acc_y), acc_y_min, acc_x_max, map_max, map_min);
	// float cutoff = powf(M_E, map_cutoff);
	// lowPass->setFc(cutoff);
	

	for(int n=0; n<context->audioFrames; n++)
	{
		// audio input
		float in = audioRead(context, n, 0);

		// compute feedback via acc_x mapping
		float feedback = map(smoothingFilter->process(acc_y), acc_y_max, acc_y_min, fdbck_min, fdbck_max);
	  
    	// Read the output from the buffer, at the location expressed by the read pointer
        float out = gDelayBuffer[gReadPointer]; // delay wet
    	
    	// Write to the circular buffer both input and feedback
        gDelayBuffer[gWritePointer] = in + feedback * out; // overwrite the buffer at the write pointer
    	
        // Increment and wrap both pointers
        gWritePointer++;
        if(gWritePointer >= gDelayBuffer.size())
        	gWritePointer = 0;
        gReadPointer++;
        if(gReadPointer >= gDelayBuffer.size())
        	gReadPointer = 0;
		
		// apply low-pass to wet
		//out = lowPass->process(out);

		out = (out+in)*0.5; // mix wet and dry

		
		// audio output
		for(int chn=0; chn<context->audioOutChannels; chn++)
			audioWrite(context, n, chn, out);

	    //printf("out %f\n", out);
	}
}

void cleanup(LDSPcontext *context, void *userData)
{
    delete smoothingFilter;
	//delete lowPass;
}