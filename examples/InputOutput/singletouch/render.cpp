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


float freqMax = 880;
float freqMin = 220;
float ampMax = 0.7;

//------------------------------------------------
float frequency = 440.0;
float amplitude = 0.0;
float phase;
float inverseSampleRate;

float maxTouchX;
float maxTouchY;

bool touchPrev;


bool setup(LDSPcontext *context, void *userData)
{
    inverseSampleRate = 1.0 / context->audioSampleRate;
	phase = 0.0;

	// retrieve touchscreen resolution
    maxTouchX = context->mtInfo->screenResolution[0]-1;
    maxTouchY = context->mtInfo->screenResolution[1]-1;

    return true;
}

void render(LDSPcontext *context, void *userData)
{
	// check if we have touch
	bool touch; 
	if(context->mtInfo->anyTouchSupported)
		touch = (multiTouchRead(context, chn_mt_anyTouch) == 1); // not all phones support anyTouch input, that checks all slots automatically
	else
		touch = (multiTouchRead(context, chn_mt_id, 0) != -1); // on most phones the slot is given an actual index only when a touch is detected

	if(touch)
	{
		float touchX = multiTouchRead(context, chn_mt_x, 0)/maxTouchX;
		float touchY = multiTouchRead(context, chn_mt_y, 0)/maxTouchY;
        
		// control frequency and amplitude with touch position
        frequency = map(touchX, 0, 1, freqMin, freqMax);
		amplitude = map(touchY, 0, 1, ampMax, 0);
	}
	else if(touchPrev) // if touch has just been released
		amplitude = 0; // silence output

	touchPrev = touch;
	

	for(int n=0; n<context->audioFrames; n++)
	{
		float out = amplitude * sinf(phase);
		phase += 2.0f * (float)M_PI * frequency * inverseSampleRate;
		while(phase > 2.0f *M_PI)
			phase -= 2.0f * (float)M_PI;
		
		for(int chn=0; chn<context->audioOutChannels; chn++)
            audioWrite(context, n, chn, out);
	}
}

void cleanup(LDSPcontext *context, void *userData)
{

}