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
#include "MonoFilePlayer.h"

#define SCREEN_REGIONS 4


// drum loop has to have same samplerate as project!
string filename = "323623__shpira__tech-drums.wav";
float bpm = 102; // bpm of drum loop
int screenRegions = SCREEN_REGIONS; // number of vertical regions we split the screen into
// each region is associated to a delay time factor and a feedback value
float delayTimeFactors[SCREEN_REGIONS] = {1, 0.5, 0.25, 0.166666}; // the base delay is the duration of a beat
float feedbacks[SCREEN_REGIONS] = {0.5, 0.6, 0.7, 0.8};

//-------------------------------------------
MonoFilePlayer player;

int anytouch_prev = 0;

// declare variables for circular buffer
std::vector<float> delayBuffer;
int writePointer = 0;
int readPointer = 0;

float amp_wet = 0;

float regionSize;

float feedback; 



bool setup(LDSPcontext *context, void *userData)
{
	// set screen on and keep it on, otherwise touches are not registered
	screenSetState(true, 1, true);

 	// Load the audio file
	if(!player.setup(filename)) 
	{
    	printf("Error loading audio file '%s'\n", filename.c_str());
    	return false;
	}


	// allocate the circular buffer to contain enough samples to cover 2 seconds
    delayBuffer.resize(2 * context->audioSampleRate);
    

	regionSize = context->mtInfo->screenResolution[0]/((float)screenRegions);

	feedback = feedbacks[0];


    return true;
}

void render(LDSPcontext *context, void *userData)
{
	float amp_dry = 0;
	
	// check if any touch is detected
	int anytouch = (multiTouchRead(context, chn_mt_id, 0)>-1);
	// if a touch is present, set amp_drylitude grater than zero
	if(anytouch == 1)
	{
		// activate dry and silence wet
		amp_dry = 0.5; // not too loud please
		amp_wet = 0;
		// if this is a new touch [rising edge]
		if(anytouch_prev == 0)
		{
			player.trigger(); // retrigger the drumloop from beginning
			std::fill(delayBuffer.begin(), delayBuffer.end(), 0); // wipe delay buffer

			// prepare new delay line
			float touchX = multiTouchRead(context, chn_mt_x, 0);
			int region = touchX/regionSize;

			// set delay time to match global bpm and region's factor
			float delayTime = delayTimeFactors[region] * 60.0/bpm; // delay time set to submultiple of beat duration [s]
			// in here, delay time is then expressed in samples
			readPointer = ( writePointer - (int)(delayTime * context->audioSampleRate ) + delayBuffer.size() ) % delayBuffer.size(); // modulo is good practice
			// set region's feedback
			feedback = feedbacks[region];
		}
	}
	else
	{
		// silence dry!
		amp_dry = 0;
		
		// if touch has just left [falling edge]
		if(anytouch_prev == 1)
			amp_wet = 0.5; // put delay into the mix, but not too loud please
	}

	// save state of touch for next check
	anytouch_prev = anytouch;


	for(int n=0; n<context->audioFrames; n++)
	{
		float dry = amp_dry*player.process(); // amo must be set hear, to mute both dry and delay input when touch is removed

		float wet = delayBuffer[readPointer]; 
    	
    	// write to the circular buffer both input and feedback
        delayBuffer[writePointer] = dry + feedback * wet; // overwrite the buffer at the write pointer
    	
        
        // increment and wrap both pointers
        writePointer++;
        if(writePointer >= delayBuffer.size())
        	writePointer = 0;
        readPointer++;
        if(readPointer >= delayBuffer.size())
        	readPointer = 0;

		float out = dry+amp_wet*wet; // mix dry and wet

		for(int chn=0; chn<context->audioOutChannels; chn++)
            audioWrite(context, n, chn, out);
	}
}

void cleanup(LDSPcontext *context, void *userData)
{

}