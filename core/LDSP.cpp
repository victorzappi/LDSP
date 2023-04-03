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
#include <cstdlib> // malloc
#include <cstring> // memset

#include "LDSP.h"

LDSPinitSettings* LDSP_InitSettings_alloc()
{
	return (LDSPinitSettings*) malloc(sizeof(LDSPinitSettings));
}

void LDSP_InitSettings_free(LDSPinitSettings* settings)
{
	free(settings);
}

void LDSP_defaultSettings(LDSPinitSettings *settings)
{
    memset(settings, 0, sizeof(*settings));

	// set default values for settings
    settings->card = 0; // only card in most devices
    settings->deviceOutNum = -1; // default will be set from hw config json file
    settings->deviceInNum = -1; // default will be set from hw config json file
    settings->periodSize = 256; // somewhat big, to make it easy for most devices
    settings->periodCount = 2; // typical lowest value supported [multiple periods probably not used at all]
    settings->numAudioOutChannels = 2; // stereo output is common
    settings->numAudioInChannels = 1; // mono input is common, cos phones have mono built-in mic
    settings->samplerate = 48000; // common standard in phones
    settings->pcmFormatString = "S16_LE"; // supported by most devices
    settings->pathOut = ""; // default is first output path in hw config json file, typically headphones
    settings->pathIn = ""; // default is first input path in hw config json file, typically built-in mic
    settings->outputOnly = 0; // full duplex engine by default
    settings->sensorsOff = 0; // sensors are on by default
    settings->ctrlInputsOff = 0; // control inputs are on by default
    settings->ctrlOutputsOff = 0; // control outputs are on by default
    settings->verbose = 1; //0; // shut up by default
    settings->deviceOutId = ""; // if not specified, extracted at run-time //VIC! remove 
    settings->deviceInId = ""; // if not specified, extracted at run-time 
}