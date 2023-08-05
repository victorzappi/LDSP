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
// the original file has beend exported as a mono track

#include "LDSP.h"
#include <vector>
#include "MonoFilePlayer.h"
#include <fft.h>

string filename = "323623__shpira__tech-drums_mono.wav";

MonoFilePlayer player;


FFT fft;
int n_fft = 1024;
int nyquist;

vector<float> inputBuffer;
vector<float> outputBuffer;
vector<float> real;
vector<float> imag;

int writePointer = 0;
int readPointer = 0;

float energyLow = 0.0;
float energyHigh = 0.0;
float energyThreshold = 0.5;

float stateLow = 0.0;
float stateHigh = 0.0;

float vibrationDuration = 100.0;



bool setup(LDSPcontext *context, void *userData)
{
	// set screen on and keep it on, otherwise touches are not registered
	screenSetState(true, 1, true);

 	// Load the audio file
	if(!player.setup(filename)) {
    	printf("Error loading audio file '%s'\n", filename.c_str());
    	return false;
	}

	// Setup the FFT object
	if(!fft.setup(n_fft)) {
		printf("Error setting up FFT with length %d\n", n_fft);
		return false;
	}

	nyquist = n_fft / 2;
	inputBuffer.resize(n_fft);

    return true;
}


void processFFT(LDSPcontext *context) {

	fft.fft(inputBuffer);

	int upperBand = nyquist / 16;
	for (int i = 0; i < nyquist; i++) {
		if (i < upperBand) {
			energyLow += fft.getRealComponent(i);
		}
		else {
			energyHigh += fft.getRealComponent(i);
		}
	}
	// Normalize energy
	energyLow /= upperBand;

	if (energyLow > energyThreshold) 
		ctrlOutputWrite(context, chn_cout_vibration, vibrationDuration);

}


void render(LDSPcontext *context, void *userData)
{	

	for(int n=0; n<context->audioFrames; n++)
	{

		float in = player.process(); 
		inputBuffer[writePointer] = in;
		writePointer++;
		// perform an fft once we have filled enough samples into the buffer
		if (writePointer > n_fft) {
			processFFT(context);
			writePointer = 0;
		}

		for(int chn=0; chn<context->audioOutChannels; chn++)
			audioWrite(context, n, chn, in);	
	}

}

void cleanup(LDSPcontext *context, void *userData)
{

}