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
#include "libraries/OscSender/OscSender.h"
#include "libraries/OscReceiver/OscReceiver.h"
#include <cmath> // sinf


float frequency = 880.0; 
float maxAmplitude = 0.2;

OscReceiver oscReceiver;
OscSender oscSender;
int localPort = 7562;
int remotePort = 7563;
const char* remoteIp = "192.168.1.97";

//------------------------------------------------

float phase;
float inverseSampleRate;

float amp;
float ampDecay = 0.9999;

float maxTouchX;
float maxTouchY;
float touchX_prev = -1;
float touchY_prev = -1;

void on_receive(oscpkt::Message* msg, const char* addr, void* arg)
{
	float ampFactor;
	msg->match("/trigger").popFloat(ampFactor);
	printf("Received trigger From %s: %f\n", addr, ampFactor);

	// make sure that requested amp factor does not clip
	if(ampFactor*maxAmplitude > 1)
		ampFactor = 1.0/maxAmplitude;

	amp = ampFactor*maxAmplitude;
}



bool setup(LDSPcontext *context, void *userData)
{
    inverseSampleRate = 1.0 / context->audioSampleRate;
	phase = 0.0;

	amp = 0;

	// setup OSC receiver and sender
	oscReceiver.setup(localPort, on_receive); // callback!
	oscSender.setup(remotePort, remoteIp);

	// retrieve touchscreen resolution
    maxTouchX = context->mtInfo->screenResolution[0]-1;
    maxTouchY = context->mtInfo->screenResolution[1]-1;

	
    return true;
}

void render(LDSPcontext *context, void *userData)
{
	float out;
	for(int n=0; n<context->audioFrames; n++)
	{
		out = amp * sinf(phase);
		phase += 2.0f * (float)M_PI * frequency * inverseSampleRate;
		while(phase > 2.0f *M_PI)
			phase -= 2.0f * (float)M_PI;

		// sine tone with exponential decay
		if(amp > 0.00001)
			amp *= ampDecay;
		
		for(int chn=0; chn<context->audioOutChannels; chn++)
            audioWrite(context, n, chn, out);
	}


	// retrieve x and y touch position
	float touchX = multiTouchRead(context, chn_mt_x, 0)/maxTouchX;
	float touchY = multiTouchRead(context, chn_mt_y, 0)/maxTouchY;

	// send via OSC only if new value
	if(touchX != touchX_prev || touchY != touchY_prev)
		oscSender.newMessage("/touch").add(touchX).add(touchY).send();

	touchX_prev = touchX;
	touchY_prev = touchY;
}

void cleanup(LDSPcontext *context, void *userData)
{
    
}