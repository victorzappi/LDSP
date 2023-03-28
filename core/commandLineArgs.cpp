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

#include <cstdio> // fprintf, NULL
#include <cstdlib> // atoi, atof...
#include <cstring> // strcpy
#include <unordered_map> // unordered_map

#include "commandLineArgs.h"
#include "tinyalsaAudio.h"
#define OPTPARSE_IMPLEMENTATION
#include "optparse.h"

using std::string;
using std::unordered_map;

unordered_map<string, int> gFormats; // extern in tinyalsaAudio.cpp



void LDSP_usage(const char *argv)
{
	// fprintf(stderr, "Passthrough from buil-in mic to headphones, via the selected devices\n");
    fprintf(stderr, "usage: %s [options]\n", argv);
    // fprintf(stderr, "options:\n");
	// fprintf(stderr, "-c | --playback-card <card number>	[0]  	The playback card\n");
    // fprintf(stderr, "-d | --playback-device <device number>	[15]	Playback card's device\n");
    // fprintf(stderr, "-C | --capture-card <card number>	[0]  	The playback card\n");
    // fprintf(stderr, "-D | --capture-device <device number>	[15]	Playback card's device\n");
	// fprintf(stderr, "-n | --playback-channels <count> 	[2]     The number of output channels\n");
	// fprintf(stderr, "-n | --capture-channels <count> 	[1]     The number of output channels\n");
    // fprintf(stderr, "-p | --period-size <size> 		[256]	The size of the PCM's period\n");
    // fprintf(stderr, "-b | --period-count <count> 		[2]	The number of PCM periods in each PCM buffer\n");
    // fprintf(stderr, "-r | --rate <rate> 			[48000]	The sample rate [Hz]\n");
    // fprintf(stderr, "-f | --format <format index> 		[0]  	The frames are in floating-point PCM\n");
    // fprintf(stderr, "Possible formats:\n");
    // for(int i=0; i<PCM_FORMAT_MAX; i++)
    // 	fprintf(stderr, "\tindex %d - %s\n", i, format_strings[i]);

	/* formats supported by all phones
	PCM_FORMAT_S16_LE = 0,
    PCM_FORMAT_S32_LE,     */
}


int LDSP_parseArguments(int argc, char** argv, LDSPinitSettings *settings)
{
 	// Create a map that associates each argument with its original position
    std::unordered_map<char*, int> orig_pos;
    for (int i = 0; i < argc; i++) {
        orig_pos[argv[i]] = i;
    }



    // first populate format map 
    for(int i=0; i<=(int)LDSP_pcm_format::MAX; i++)
    {
        LDSP_pcm_format format = (LDSP_pcm_format::_enum)LDSP_pcm_format::_from_index(i);
        string name = format._to_string();
        int value = (int)format._from_string(format._to_string());

        gFormats[name] = value;
    }

	int c;
	struct optparse opts;
	struct optparse_long long_options[] = {
		{ "card",         		'c', OPTPARSE_REQUIRED },
		{ "output-device-num",	'd', OPTPARSE_REQUIRED },
		{ "input-device-num",   'D', OPTPARSE_REQUIRED },
		{ "output-device-id",	's', OPTPARSE_REQUIRED },
		{ "input-device-id",    'S', OPTPARSE_REQUIRED },
		{ "period-size",  		'p', OPTPARSE_REQUIRED },
		{ "period-count", 		'b', OPTPARSE_REQUIRED },
		{ "output-channels", 	'n', OPTPARSE_REQUIRED },
		{ "input-channels",  	'N', OPTPARSE_REQUIRED },
		{ "samplerate",    		'r', OPTPARSE_REQUIRED },
		{ "format",       		'f', OPTPARSE_REQUIRED },
		{ "output-path",       	'o', OPTPARSE_REQUIRED },
		{ "input-path",       	'i', OPTPARSE_REQUIRED },
		{ "output-only",       	'O', OPTPARSE_NONE },
		{ "verbose",         	'v', OPTPARSE_NONE     },
		{ "help",         		'h', OPTPARSE_NONE     },
		{ 0, 0, OPTPARSE_NONE }
	};

	int retVal = 0;

	optparse_init(&opts, argv);
	while ((c = optparse_long(&opts, long_options, NULL)) != -1) {
		//TODO remove arguments that are parsed
		switch (c) 
        {
			case 'c':
				settings->card = atoi(opts.optarg);
				break;
			case 'd':
				settings->deviceOutNum = atoi(opts.optarg);
				break;
			case 'D':
				settings->deviceInNum = atoi(opts.optarg);
				break;
			case 'p':
				settings->periodSize = atoi(opts.optarg);
				break;
			case 'b':
				settings->periodCount = atoi(opts.optarg);
				break;
			case 'n':
				settings->numAudioOutChannels = atoi(opts.optarg);
				break;
			case 'N':
				settings->numAudioInChannels = atoi(opts.optarg);
				break;
			case 'r':
				settings->samplerate = atoi(opts.optarg);
				break;
			case 'f':
				settings->pcmFormatString = opts.optarg;
			 	break;
			case 'o':
				settings->pathOut = opts.optarg;
			 	break;
			case 'i':
				settings->pathIn = opts.optarg;
			 	break;
			case 'O':
				settings->outputOnly = 1;
			 	break;
			case 'v':
				settings->verbose = 1;
			 	break;
			case 'h': 
			//case '?': // we want extra arguments to be accepted and possibly managed by user
				LDSP_usage(argv[0]);
				retVal = -1;
			default:
				break;
		}
	}

	
	// revert arguments back to original order, so that user can apply further parsing
    std::sort(argv, argv + argc, [&](char* a, char* b) {
        return orig_pos[a] < orig_pos[b];
    });

	return retVal;
}