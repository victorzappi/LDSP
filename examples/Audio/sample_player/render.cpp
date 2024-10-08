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

// This code example uses this sound from freesound:
// Simple Lofi Vinyl E-Piano Loop 95 BPM.wav by holizna -- https://freesound.org/s/629178/ -- License: Creative Commons 0
// the original file has been exported as a mono track and resampled at 48 kHz

#include "LDSP.h"
#include "MonoFilePlayer.h"


// drum loop has to have same samplerate as project!
string filename = "629178__holizna__simple-lofi-vinyl-e-piano-loop-95-bpm_mono_48k.wav";
//-------------------------------------------

MonoFilePlayer player;


bool setup(LDSPcontext *context, void *userData)
{
	bool loop = true;
	bool autostart = true;
 	
	// load the audio file
	if( !player.setup(filename, loop, autostart) ) 
	{
    	printf("Error loading audio file '%s'\n", filename.c_str());
    	return false;
	}

    return true;
}

void render(LDSPcontext *context, void *userData)
{
	for(int n=0; n<context->audioFrames; n++)
	{
		// get next sample from file
		float out = player.process(); 

		for(int chn=0; chn<context->audioOutChannels; chn++)
            audioWrite(context, n, chn, out);
	}
}

void cleanup(LDSPcontext *context, void *userData)
{

}