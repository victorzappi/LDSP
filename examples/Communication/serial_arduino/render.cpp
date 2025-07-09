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
#include <cmath> // sin
#include <unistd.h> // usleep

#include "libraries/Arduino/Arduino.h"

#define NUM_OF_INPUTS 2

float frequency = 440.0;
float amplitude = 0.1;

float lfo_frequency = 2.0;
float lfo_freq_min = 1.0;
float lfo_freq_max = 20.0;

float amp_thresh = 0.3;

//------------------------------------------------
float phase;
float lfo_phase;
float prev_lfo_amp;
float inverseSampleRate;

Arduino arduino;

bool setup(LDSPcontext *context, void *userData)
{
	screenSetState(true, 1.0, true);
	printf("\nOn many phones Arduino is detected only if the screen is unlocked!\n\n");
	printf("Waiting for automatic screen unlock...\n");
	printf("(if the screen does not unlock automatically in the next 3 seconds, please do it manually)\n");
	usleep(3000000);
	printf("...done waiting!\n\n");

    arduino.listPorts();

	// Arduino sketch and LDSP project should agree on the number integers sent to LDSP via serial
	arduino.setNumOfInputs(NUM_OF_INPUTS); // in this case, we receive packets of two integers, e.g., they could be data from Analog In 0 and 1!
	// packets must be formatted as "...%d %d\n", according to the number of inputs

	if(!arduino.open(115200)) // let's go for a higher baudrate than the standard 9600! we are sending and receiving serial data streams
		return false;

    inverseSampleRate = 1.0 / context->audioSampleRate;
	phase = 0.0;
	lfo_phase = 0.0;
	prev_lfo_amp = 0.0;

    return true;
}


void render(LDSPcontext *context, void *userData)
{
	// read from Arduino, we expect packets of 2 integers, formatted as "%d %d\n"
	int inputs[NUM_OF_INPUTS];
	int retVal = arduino.read(inputs); // the read functions unpacks them for us!
	if(retVal > -1) // our inputs are stored into the array, but the first input is also returned as retval
		lfo_frequency = map(inputs[0], 0, 1023, lfo_freq_min, lfo_freq_max); // use the first input to control LFO
	// we could also do something with input[1]... but we are lazy and we don't 

	float lfo_amp;
	for(int n=0; n<context->audioFrames; n++)
	{
		// LFO
		lfo_amp = (1 + sinf(lfo_phase)) / 2.0;
		lfo_phase += 2.0f * (float)M_PI * lfo_frequency * inverseSampleRate;
		while(lfo_phase > 2.0f *M_PI)
			lfo_phase -= 2.0f * (float)M_PI;

		// sine
		float out = lfo_amp * amplitude * sinf(phase);
		phase += 2.0f * (float)M_PI * frequency * inverseSampleRate;
		while(phase > 2.0f *M_PI)
			phase -= 2.0f * (float)M_PI;
		
		for(int chn=0; chn<context->audioOutChannels; chn++)
            audioWrite(context, n, chn, out);
	}

	// write to Arduino, send 1 and 0 when LFO goes above and below threshold, respectively
	if(lfo_amp >= amp_thresh && prev_lfo_amp < amp_thresh)
		arduino.write(1.0); // write sends a single float
	else if(lfo_amp <= amp_thresh && prev_lfo_amp > amp_thresh)
		arduino.write(0.0);

	prev_lfo_amp = lfo_amp;
}

void cleanup(LDSPcontext *context, void *userData)
{

}