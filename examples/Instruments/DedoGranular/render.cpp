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
#include "libraries/Granular/Granular.h"

using std::vector;

#define GRANULATORS_NUM 2

int numGrains = 4;
string file[GRANULATORS_NUM] = {"./audiofileA.wav", "audiofileB.wav"}; // choose two audiofiles and put them in the project directory; set LDSP sample rate to match the files' rate
float amplitude[2] = {1.0, 0.3};
float playbackSpeed = 1;
int grainSize = 1000;
float grainSizeRandomness = 0.85;
float filePositionRandomness = 0.05;
//float probability = 0.5;

//------------------------------------------------
vector<Granular> granulators;

float maxTouchX;
float maxTouchY;

float crossfade = 0;

bool setup(LDSPcontext *context, void *userData)
{

    for(int i=0; i<GRANULATORS_NUM; i++)
    {
        Granular granular;
        if (!granular.setup(numGrains, context->audioSampleRate, file[i])) 
        {
            printf("failed to setup granular");
            return false;
        }

        granular.setPlaybackSpeed(playbackSpeed);
        granular.setGrainSizeRandomness(grainSizeRandomness);
        granular.setFilePositionRandomness(filePositionRandomness);
        //granular.setProbability(probability);
        granulators.push_back(granular);
    }

    maxTouchX = context->mtInfo->screenResolution[0]-1;
    maxTouchY = context->mtInfo->screenResolution[1]-1;
   
    return true;
}

void render(LDSPcontext *context, void *userData)
{
    // not all phones support the 'any touch' input
    //int touch = multiTouchRead(context, chn_mt_anyTouch); 
    // so we approximate it, by checking the id assigned to the first slot
    bool touch = (multiTouchRead(context, chn_mt_id, 0) != -1);
    
    if(touch)
    {
        float filePosition = multiTouchRead(context, chn_mt_y, 0)/maxTouchY;

        granulators[0].setFilePosition(filePosition);
        granulators[1].setFilePosition(filePosition);

        granulators[0].setGrainSizeMS(grainSize);
        granulators[1].setGrainSizeMS(grainSize);

        float touchX = multiTouchRead(context, chn_mt_x, 0)/maxTouchX;
        touchX = constrain(touchX, 0.2, 0.8);
        crossfade = map(touchX, 0.2, 0.8, 0, 1);
    }
    else
    {
        granulators[0].setGrainSizeMS(0);
        granulators[1].setGrainSizeMS(0);
    }

	for(unsigned int n = 0; n < context->audioFrames; n++) {

		float out = (1-crossfade)*amplitude[0]*granulators[0].process()+crossfade*amplitude[1]*granulators[1].process();

		for(unsigned int channel = 0; channel < context->audioOutChannels; channel++) {
			audioWrite(context, n, channel, out);
		}
	}
}

void cleanup(LDSPcontext *context, void *userData)
{

}