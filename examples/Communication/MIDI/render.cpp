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

void midiMessageCallback(MidiChannelMessage message, void* arg) 
{
	if(arg != NULL) 
	{
		printf("Message from midi port %s: ", (const char*) arg);
		printf("channel %d, byte0 %d, byte1 %d\n", message.getChannel(), message.getDataByte(0), message.getDataByte(1));
		
		// int chn = message.getChannel(); // coudl be useful in other cases!
		int type = message.getType();
		int note, vel;
		//int cc, val; // midi ccs are parsed too! type is kmmControlChange

		// in Midi.h, look for midiMessageType to see all types of messages that are parsed
		
		// to be thread-safe, all frequency and amp should be atomics
		// but let's keep this example as simple as possible (;
		if(type==kmmNoteOn) 
		{
			// note to frequency mapping
			note = message.getDataByte(0);
			frequency = 440.0 * powf(2, (note-69)/12.0); // from midi note number to freqency
			
			// velocity to amp mapping
			vel = message.getDataByte(1); 
			amp = 0.3*(vel/127.0) + 0.7; // let's use vel to control only 30% of the amp
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

	// set up midi object to read from the midi controller connected to specified midi port
	if(midi.readFrom(midiPort)==-1) 
	{
		printf("Cannot open MIDI device %s\n", midiPort);
		return false;
	}

	// we activate the parser mechanism, so that each time we receive a message, bytes are correctly interpreted automatically!
	midi.enableParser(true);
	midi.setParserCallback(midiMessageCallback, (void*) midiPort); // this is the callback that will be triggered when a new message arrives and is parsed

	//TODO add midi.writeTo() example code

    return true;
}

void render(LDSPcontext *context, void *userData)
{
	for(int n=0; n<context->audioFrames; n++)
	{
		// frequency and amplitude are controlled via midi, from within the callback
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