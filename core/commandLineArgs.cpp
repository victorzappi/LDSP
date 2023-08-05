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
    fprintf(stderr, "Usage: %s [options]\n", argv);
    fprintf(stderr, "options, with default values in brackets:\n");
	fprintf(stderr, "-c | --card <card number>\t\t\tSound card [0]\n");
    fprintf(stderr, "-d | --output-device-num <device number>\tCard's playback device number\n");
    fprintf(stderr, "-D | --input-device-num <device number>\t\tCard's capture device number\n");
	fprintf(stderr, "-d | --output-device-id <id>\t\t\tCard's playback device id (name)\n");
    fprintf(stderr, "-D | --input-device-id <id>\t\t\tCard's capture device id (name)\n");
	fprintf(stderr, "-p | --period-size <size>\t\t\tNumber of frames per each audio block [256]\n");
	fprintf(stderr, "-b | --period-count <count>\t\t\tNumber of audio blocks the audio ring buffer can contain [2]\n");
	fprintf(stderr, "-n | --output-channels <count>\t\t\tNumber of playback audio channels [2]\n");
	fprintf(stderr, "-N | --input-channels <count>\t\t\tNumber of capture audio channels [1]\n");
	fprintf(stderr, "-r | --samplerate <rate>\t\t\tSample rate in Hz [48000]\n");
	fprintf(stderr, "-f | --format <format index>\t\t\tIndex of format of samples [0]\n");
    fprintf(stderr, "\tPossible formats:\n");
    for(int f=0; f<LDSP_pcm_format::MAX; f++)
	{
		LDSP_pcm_format ff = (LDSP_pcm_format::_enum)LDSP_pcm_format::_from_index(f);
        string name = ff._to_string();
		printf("\t\tindex %d - %s\n", f, name.c_str());
	}
	fprintf(stderr, "-o | --output-path <path name>\t\t\tOutput mixer path\n");
	fprintf(stderr, "-i | --input-path <path name>\t\t\tInput mixer path\n");
	fprintf(stderr, "-O | --output-only\t\t\t\tDisables captures [capture enabled]\n");
	fprintf(stderr, "-P | --sensors-off\t\t\t\tDisables sensors [sensors enabled]\n");
	fprintf(stderr, "-Q | --ctrl-inputs-off\t\t\t\tDisables control inputs [control inputs enabled]\n");
	fprintf(stderr, "-R | --ctrl-outputs-off\t\t\t\tDisables control outputs [control outputs enabled]\n");
	fprintf(stderr, "-A | --perf-mode-off\t\t\t\tDisables CPU's governor peformance mode [performance mode enabled]\n");
	fprintf(stderr, "-v | --verbose\t\t\t\t\tPrints all phone's info, current settings main function calls [off]\n");
	fprintf(stderr, "-h | --help\t\t\t\t\tPrints this and exits [off]\n");
}


int LDSP_parseArguments(int argc, char** argv, LDSPinitSettings *settings)
{
 	// Create a map that associates each argument with its original position
    unordered_map<char*, int> orig_pos;
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
		{ "sensors-off",       	'P', OPTPARSE_NONE },
		{ "ctrl-inputs-off",    'Q', OPTPARSE_NONE },
		{ "ctrl-outputs-off",   'R', OPTPARSE_NONE },
		{ "perf-mode-off",      'A', OPTPARSE_NONE },
		{ "verbose",         	'v', OPTPARSE_NONE     },
		{ "help",         		'h', OPTPARSE_NONE     },
		{ 0, 0, OPTPARSE_NONE }
	};

	int retVal = 0;
	int notRec = 0;

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
			case 'P':
				settings->sensorsOff = 1;
			 	break;
			case 'Q':
				settings->ctrlInputsOff = 1;
			 	break;
			case 'R':
				settings->ctrlOutputsOff = 1;
			case 'A':
				settings->perfModeOff = 1;
			 	break;
			case 'v':
				settings->verbose = 1;
			 	break;
			case 'h': 
			//case '?': // we want extra arguments to be accepted and possibly managed by user
				LDSP_usage(argv[0]);
				retVal = -1;
				break;
			default:
				notRec++;
				break;
		}
	}

	if(notRec > 0)
	{ 
		string cc = "argument";
		if(notRec > 1)
			cc = "arguments";
		printf("\n\t%d %s not recognized by main parser\n", notRec, cc.c_str());
	}

	
	// revert arguments back to original order, so that user can apply further parsing
    std::sort(argv, argv + argc, [&](char* a, char* b) {
        return orig_pos[a] < orig_pos[b];
    });

	return retVal;
}