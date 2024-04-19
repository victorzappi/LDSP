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
#include "MyArduino.h"



#define INPUT_SIZE 2

float frequency = 440.0;
float amplitude = 0.1;

float lfo_frequency = 2.0;
float lfo_freq_min = 1.0;
float lfo_freq_max = 20.0;

float amp_thresh = 0.3;

//------------------------------------------------
float phase;
float lfo_phase;

float inverseSampleRate;

MyArduino arduino;

void arduinoCallback(int* inputData, int inputNum){
	int num = 0;
	for(int i=0;i<inputNum;i++){
		printf("%d,",inputData[i]);
		num++;
		if(!(num % INPUT_SIZE)){
			num = 0;
			printf("\t");
		}
	}
	printf("\n\n");
	
}

bool setup(LDSPcontext *context, void *userData)
{
	screenSetState(true, 1.0, true);
	printf("\nOn many phones Arduino is detected only if the screen is unlocked!\n\n");
	printf("Waiting for automatic screen unlock...\n");
	printf("(if the screen does not unlock automatically in the next 3 seconds, please do it manually)\n");
	usleep(3000000);
	printf("...done waiting!\n\n");

	arduino.listPorts();
	if(!arduino.open(115200)) // let's go for a higher baudrate than the standard 9600! we are sending and receiving serial data streams
		return false;

	arduino.startRead(arduinoCallback, 2, ',');


    inverseSampleRate = 1.0 / context->audioSampleRate;
	phase = 0.0;
	lfo_phase = 0.0;

    return true;
}


void render(LDSPcontext *context, void *userData)
{
	

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

}

void cleanup(LDSPcontext *context, void *userData)
{
	arduino.close();
}