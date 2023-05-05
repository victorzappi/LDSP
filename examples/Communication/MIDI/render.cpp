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
#include <math.h> // sin

#include "Midi.h"

float frequency = 440.0;
float maxAmp = 0.3;

Midi midi;

//------------------------------------------------
float phase;
float inverseSampleRate;

float amp = 0.0;

void midiMessageCallback(MidiChannelMessage message, void* arg) {
	if(arg != NULL) {
		printf("Message from midi port %s: ", (const char*) arg);
		printf("channel %d, byte0 %d, byte1 %d\n", message.getChannel(), message.getDataByte(0), message.getDataByte(1));
		
		// int chn = message.getChannel();
		int type = message.getType();
		int note, vel;
		//int cc, val;
		

		if(type==kmmNoteOn) 
		{
			amp = 1;
			note = message.getDataByte(0);
			frequency = 440.0 * powf(2, (note-69)/12.0);
			//vel = message.getDataByte(1);
		}
		else if(type==kmmNoteOff) 
			amp = 0;
	}
}



bool setup(LDSPcontext *context, void *userData)
{
    inverseSampleRate = 1.0 / context->audioSampleRate;
	phase = 0.0;

	const char* midiPort = midi.getDefaultMidiPort();

	// set up midi to read and write from midi controller
	if(midi.readFrom(midiPort, 5)==-1) {
		printf("Cannot open MIDI device %s\n", midiPort);
		return false;
	}

	midi.enableParser(true);
	midi.setParserCallback(midiMessageCallback, (void*) midiPort);

    return true;
}

void render(LDSPcontext *context, void *userData)
{
	for(int n=0; n<context->audioFrames; n++)
	{
		float out = amp * maxAmp * sinf(phase);
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