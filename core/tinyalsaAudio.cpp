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


//VIC we are using tinyalsa implementation from Android 4.0, 4.0.1, 4.0.2 [ICE_CREAM_SANDWICH], API level 14
// apparently, this was the first android release that used tinyalsa 
// it sounds crazy, but by doing so we are maximizing the compatibility with old phones that still have good chips but cannot updgrade to higher android versions
// caveats:
// _we stick to most basic subset of tinyalsa lib functions [e.g., no params checks]
// _we need to wrap enum pcm_format into a larger and generic container
// because ICE_CREAM_SANDWICH has very few enum entries... and we need all those listed in version of tinyalsa of latest Android [13] to be fully compatible with newer phones too
// in tinyalsaAudio.h and tinyalsaExtension.cpp we are re-implementing functions that are missing in old tinyalsa implementation but would be handy to use [e.g., params checks, format check]
// luckily, we have almost all the needed functions to run a full-duplex pcm! 
// the newest version of tinyalsa deprecated some basic functions that we are using, but even the latest Android [13] is still tied to an old implementation where deprecation is yet to happen
// when these new implmentaitons are adopted in android, we will have to add the new functions that replace the deprecated ones to tinyalsaAudio.h/tinyalsaExtension.cpp
// hopefully this will happen in a far future...

#include <unordered_map> // unordered_map
#include <thread> // number of cpus
#include <dirent.h> // browse dirs

#include "LDSP.h"
#include "tinyalsaAudio.h"
#include "mixer.h"
// /#include "tinyAlsaExtension.h"
#include "priority_utils.h"
#include "sensors.h"
#include "ctrlInputs.h"
#include "ctrlOutputs.h"

using std::string;
using std::ifstream;
using std::ofstream;

#define GOVERNOR_BASE_DIR "/sys/devices/system/cpu/"


bool volatile gShouldStop = false; // flag that tells the audio process to stop, extern in ctrlInputs.cpp

bool fullDuplex;

// to easily access the wrapper around pcm_format enum
extern unordered_map<string, int> gFormats;

bool audioVerbose = false;

vector<string> governors;

LDSPpcmContext pcmContext;
LDSPinternalContext intContext;
LDSPcontext* userContext = nullptr;


// function pointers set up in initFormatFunctions()
// capture
int (*byteCombine)(unsigned char*, audio_struct*);
int byteCombine_littleEndian(unsigned char*, audio_struct*);
int byteCombine_bigEndian(unsigned char*, audio_struct*);
// playback
void (*fromRawToFloat)(audio_struct*);
void fromRawToFloat_int(audio_struct*);
void fromRawToFloat_float32(audio_struct*);


void (*byteSplit)(unsigned char*, int, audio_struct*);
void byteSplit_littleEndian(unsigned char*, int, audio_struct*);
void byteSplit_bigEndian(unsigned char*, int, audio_struct*);

void (*fromFloatToRaw)(audio_struct*);
void fromFloatToRaw_int(audio_struct*);
void fromFloatToRaw_float32(audio_struct*);


// non-exposed functions
void initAudioParams(LDSPinitSettings *settings, audio_struct **audioStruct, bool is_playback);
void cleanupAudioParams(LDSPpcmContext *pcmContext);
int initFormatFunctions(string format);
int initPcm(audio_struct *audio_struct_p, audio_struct *audio_struct_c);
void cleanupPcm(LDSPpcmContext *pcmContext);
int initLowLevelAudioStruct(audio_struct *audio_struct);
void deallocateLowLevelAudioStruct(audio_struct *audio_struct);
void cleanupLowLevelAudioStruct(LDSPpcmContext *pcmContext);
void setGovernorMode();
void resetGovernorMode();
void *audioLoop(void*); 


int LDSP_initAudio(LDSPinitSettings *settings, void *userData)
{
    if(!settings)
		return -1;
    
	fullDuplex = !settings->outputOnly;
    audioVerbose = settings->verbose;
	

    if(audioVerbose)
        printf("\nLDSP_initAudio()\n");

	//TODO deactivate audio server

    initAudioParams(settings, &pcmContext.playback, true);
    initAudioParams(settings, &pcmContext.capture, false);

	if(initFormatFunctions(settings->pcmFormatString)<0) // format is the same for playback and capture
    {
        cleanupAudioParams(&pcmContext);
		return  -1;
    }

    if(initPcm(pcmContext.playback, pcmContext.capture)<0)
    {
        cleanupPcm(&pcmContext);
        cleanupAudioParams(&pcmContext);
        return  -2;
    }
    if(initLowLevelAudioStruct(pcmContext.playback)<0)
    {
        // try partial deallocation
        deallocateLowLevelAudioStruct(pcmContext.playback);
        return -3;
    }
	if(initLowLevelAudioStruct(pcmContext.capture)<0)
	{
		// try partial deallocation
		deallocateLowLevelAudioStruct(pcmContext.capture);
		return -3;
	}

	// activate performance governor
	setGovernorMode();
	
	gFormats.clear();

	// init context
    intContext.audioIn = pcmContext.capture->audioBuffer;
	intContext.audioOut = pcmContext.playback->audioBuffer;
	intContext.audioFrames = pcmContext.playback->config.period_size;
	intContext.audioInChannels = pcmContext.capture->config.channels;
	intContext.audioOutChannels = pcmContext.playback->config.channels;
	intContext.audioSampleRate = (float)pcmContext.playback->config.rate;
	userContext = (LDSPcontext*)&intContext;

    return 0;
}

int LDSP_startAudio()
{	
	if(audioVerbose)
	{
    	printf("\nLDSP_startAudio()\n");
		printf("Starting audio thread...\n");
	}


	setup(userContext, 0);

	pthread_t audioThread;
	if( pthread_create(&audioThread, nullptr, audioLoop, nullptr) ) 
	{
		fprintf(stderr, "Error:unable to create thread\n");
		return -1;
	}

	// wait for end of thread
	pthread_join(audioThread, nullptr);

	cleanup(userContext, 0);

	return 0;
}

void LDSP_cleanupAudio()
{
    if(audioVerbose)
        printf("LDSP_cleanupAudio()\n");

    cleanupLowLevelAudioStruct(&pcmContext);
    cleanupPcm(&pcmContext);
    cleanupAudioParams(&pcmContext); 

	resetGovernorMode();
	
	//TODO reactivate audio server
}

void LDSP_requestStop()
{
	gShouldStop = true;
}


//------------------------------------------------------------------------------------
int checkAllCardParams(audio_struct *audioStruct);

void initAudioParams(LDSPinitSettings *settings, audio_struct **audioStruct, bool is_playback)
{
    *audioStruct = (audio_struct*) malloc(sizeof(audio_struct));
    
    (*audioStruct)->card = settings->card;
    if(is_playback)
    {
        (*audioStruct)->flags = PCM_OUT;
        (*audioStruct)->device = settings->deviceOutNum;
        (*audioStruct)->config.channels = settings->numAudioOutChannels;
    }
    else
    {
        (*audioStruct)->flags = PCM_IN;
        (*audioStruct)->device = settings->deviceInNum;
		if(fullDuplex)
        	(*audioStruct)->config.channels = settings->numAudioInChannels;
		else
			(*audioStruct)->config.channels = 0;
    }

    (*audioStruct)->config.period_size = settings->periodSize;
    (*audioStruct)->config.period_count =settings->periodCount;
    (*audioStruct)->config.rate = settings->samplerate;
    (*audioStruct)->config.format = (pcm_format) gFormats[settings->pcmFormatString];
    (*audioStruct)->config.silence_threshold = 1; //settings->periodSize * settings->periodCount; 
    (*audioStruct)->config.silence_size = 0;
    (*audioStruct)->config.start_threshold = 1; //settings->periodSize;
	(*audioStruct)->config.stop_threshold = 0;

    (*audioStruct)->pcm = nullptr;
    (*audioStruct)->fd = (int)NULL; // needs C's NULL

    if( audioVerbose && 
		( is_playback || (!is_playback && fullDuplex) )
	)
    {
        if(is_playback)
            printf("Playback");    
        else
            printf("Capture");    
        printf(" card %d, device %d\n", (*audioStruct)->card, (*audioStruct)->device);
        if(is_playback)
            printf("\tid: %s\n", settings->deviceOutId.c_str());   
        else
            printf("\tid: %s\n", settings->deviceInId.c_str());


		checkAllCardParams((*audioStruct));

        printf("Requested params:\n");
        printf("\tPeriod size: %d\n", (*audioStruct)->config.period_size);
        printf("\tPeriod count: %d\n", (*audioStruct)->config.period_count);
        printf("\tChannels: %d\n", (*audioStruct)->config.channels);
        printf("\tRate: %d\n", (*audioStruct)->config.rate);
        printf("\tFormat: %s\n", settings->pcmFormatString.c_str());
        printf("\n");
    }
}

void cleanupAudioParams(LDSPpcmContext *pcmContext)
{
    if(pcmContext->playback != nullptr)
        free(pcmContext->playback);
    if(pcmContext->capture != nullptr)
        free(pcmContext->capture);
}

int initFormatFunctions(string format)
{
    int f = gFormats[format];
    
	int err = 0;
    switch(f)
    {
    	case LDSP_pcm_format::S16_LE:
    	case LDSP_pcm_format::S32_LE:
    	case LDSP_pcm_format::S8:  // but it doesn't really matter, single byte no need to split/combine
    	case LDSP_pcm_format::S24_LE:
    	case LDSP_pcm_format::S24_3LE:
			byteCombine = byteCombine_littleEndian;
    		byteSplit = byteSplit_littleEndian;
			fromRawToFloat = fromRawToFloat_int;
    		fromFloatToRaw = fromFloatToRaw_int;
    		break;
    	case LDSP_pcm_format::S16_BE:
    	case LDSP_pcm_format::S32_BE:
    	case LDSP_pcm_format::S24_BE:
    	case LDSP_pcm_format::S24_3BE:
    	    byteCombine = byteCombine_bigEndian;
			byteSplit = byteSplit_bigEndian;
			fromRawToFloat = fromRawToFloat_int;
    	    fromFloatToRaw = fromFloatToRaw_int;
    		break;
    	case LDSP_pcm_format::FLOAT_LE:
			byteCombine = byteCombine_littleEndian;
    		byteSplit = byteSplit_littleEndian;
			fromRawToFloat = fromRawToFloat_float32;
    		fromFloatToRaw = fromFloatToRaw_float32;
    		break;
    	case LDSP_pcm_format::FLOAT_BE:
		    byteCombine = byteCombine_bigEndian;
    	    byteSplit = byteSplit_bigEndian;
			fromRawToFloat = fromRawToFloat_float32;
    	    fromFloatToRaw = fromFloatToRaw_float32;
			break;
    	case LDSP_pcm_format::MAX:
    	default:
    		fprintf(stderr, "Invalid format %s\n", format.c_str());
    		format = LDSP_pcm_format::MAX;
    		err = -1;
    		break;
    }

	return err;
}

int initPcm(audio_struct *audio_struct_p, audio_struct *audio_struct_c)
{   	
    // open playback card 

    pcm_config *config_p = (pcm_config *)&audio_struct_p->config;
    
    // open playback pcm handle
	audio_struct_p->pcm = pcm_open(audio_struct_p->card, audio_struct_p->device, audio_struct_p->flags, config_p);
	audio_struct_p->fd = pcm_is_ready(audio_struct_p->pcm);
	if (!audio_struct_p->pcm || !audio_struct_p->fd) 
	{
		fprintf(stderr, "Failed to open playback pcm %u,%u. %s\n", audio_struct_p->card, audio_struct_p->device, pcm_get_error(audio_struct_p->pcm));
		return -1;
	}

    //VIC only added in later versions of tinyalsa, apparently not needed though
	// last touch for playback
	// if(pcm_prepare(audio_struct_p->pcm) < 0) 
	// {
	// 	fprintf(stderr, "Pcm prepare error: %s\n", pcm_get_error(audio_struct_p->pcm));
	// 	return -2;
	// }

	audio_struct_p->frameBytes = pcm_frames_to_bytes(audio_struct_p->pcm, config_p->period_size); //VIC note that we are using period size, not buffer size
    
    if(audioVerbose)
	    printf("Playback audio device opened\n");



	if(fullDuplex)
	{
		// open capture card 
		pcm_config *config_c = (pcm_config *)&audio_struct_c->config;

		// open capture pcm handle
		audio_struct_c->pcm = pcm_open(audio_struct_c->card, audio_struct_c->device, audio_struct_c->flags, config_c);
		audio_struct_c->fd = pcm_is_ready(audio_struct_c->pcm);
		if(!audio_struct_c->pcm || !audio_struct_c->fd) 
		{
			fprintf(stderr, "Failed to open capture pcm %u,%u. %s\n", audio_struct_c->card, audio_struct_c->device, pcm_get_error(audio_struct_c->pcm));
			return -1;
		}

		audio_struct_c->frameBytes =  pcm_frames_to_bytes(audio_struct_c->pcm, config_c->period_size); //VIC note that we are using period size, not buffer size

		if(audioVerbose)
			printf("Capture audio device opened\n");


		if(LDSP_pcm_link(audio_struct_p, audio_struct_c) <0)
			return -2;
		
		if(audioVerbose)
			printf("Playback and Capture audio device linked!\n");
	}

    return 0;
}

void closePcm(audio_struct* audio_struct, bool is_playback)
{
	// close pcm handle
	if(audio_struct->pcm != nullptr)
	{
		if (pcm_is_ready(audio_struct->pcm))
		{
			if(!is_playback)
            {
			    LDSP_pcm_unlink(audio_struct);
                if(audioVerbose)
                    printf("Playback audio device unlinked from Capture audio device!\n");
            }

			pcm_close(audio_struct->pcm);
            if(audioVerbose)
            {
                if(is_playback)
                    printf("Playback");    
                else
                    printf("Capture");    
			    printf(" audio device closed\n");
            }
		}
	}
}

void cleanupPcm(LDSPpcmContext *pcmContext)
{
    closePcm(pcmContext->playback, true);
	if(fullDuplex)
    	closePcm(pcmContext->capture, false);
}

int initLowLevelAudioStruct(audio_struct *audio_struct)
{
	audio_struct->rawBuffer = nullptr;
	audio_struct->audioBuffer = nullptr;
    
	int channels = audio_struct->config.channels;
	// to cover the case of capture when non full duplex engine
	if(channels <=0)
		channels = 1;

	audio_struct->numOfSamples = channels*audio_struct->config.period_size;
	
	unsigned int formatBits;

	// allocate buffers
	audio_struct->rawBuffer = malloc(audio_struct->frameBytes);
	if(!audio_struct->rawBuffer)
	{
		fprintf(stderr, "Could not allocate rawBuffer\n");
		return -1;
	}

	audio_struct->audioBuffer = (float*)malloc(sizeof(float)*audio_struct->numOfSamples);
	if(!audio_struct->audioBuffer)
	{
		fprintf(stderr, "Could not allocate audioBuffer\n");
		return -2;
	}
	memset(audio_struct->audioBuffer, 0, audio_struct->numOfSamples*sizeof(float)); // quiet please!

	// finish audio initialization
	//formatBits = LDSP_pcm_format_to_bits((int)audio_struct->config.format); // replaces/implements pcm_format_to_bits(), that is missing from old tinyalsa
	formatBits = pcm_format_to_bits(audio_struct->config.format);
	audio_struct->formatBits = formatBits;
	audio_struct->scaleVal = (1 << (formatBits - 1)) - 1;
	audio_struct->physBps = formatBits / 8;  // size in bytes of the format var type used to store sample
	// different than this, i.e., number of bytes actually used within that format var type!
	if(audio_struct->config.format != gFormats["S24_3LE"] && audio_struct->config.format != gFormats["S24_3BE"]) // these vars span 32 bits, but only 24 are actually used! -> 3 bytes
		audio_struct->bps = audio_struct->physBps;
	else
		audio_struct->bps = 3;

	// this is used for capture only
	// we compute the mask necessary to complete the two's complment of received raw samples
	audio_struct->mask = 0x00000000;
	for (int i = 0; i<sizeof(int)-audio_struct->bps; i++)
		audio_struct->mask |= (0xFF000000 >> i*8);
			
	return 0;
}


void deallocateLowLevelAudioStruct(audio_struct *audio_struct)
{
    // deallocate buffers
	if(audio_struct->rawBuffer != nullptr)
		free(audio_struct->rawBuffer);
	if(audio_struct->audioBuffer != nullptr) 
		free(audio_struct->audioBuffer);
}

void cleanupLowLevelAudioStruct(LDSPpcmContext *pcmContext)
{
    deallocateLowLevelAudioStruct(pcmContext->playback);
    deallocateLowLevelAudioStruct(pcmContext->capture);
}

void *audioLoop(void*)
{
	// set thread niceness
 	set_niceness(-20, audioVerbose); // only necessary if not real-time

	// set thread priority
	set_priority(0, audioVerbose);

	while(!gShouldStop)
	{
		if(fullDuplex)
		{
			if(pcm_read(pcmContext.capture->pcm, pcmContext.capture->rawBuffer, pcmContext.capture->frameBytes)!=0)
			{
				fprintf(stderr, "Capture error, aborting...\n");
			}

			fromRawToFloat(pcmContext.capture);
		}

		readSensors();
		readCtrlInputs();

		render(userContext, 0);

		fromFloatToRaw(pcmContext.playback);

		if(pcm_write(pcmContext.playback->pcm, pcmContext.playback->rawBuffer, pcmContext.playback->frameBytes)!=0)
		{
			fprintf(stderr, "Playback error, aborting...\n");
		}

		writeCtrlOutputs();
	}

	if(audioVerbose)
		printf("Audio thread stopped!\n\n");

	return (void *)0;
}


//VIC on android, it seems that interleaved is the only way to go
void fromRawToFloat_int(audio_struct *audio_struct) 
{
	unsigned char *sampleBytes = (unsigned char *)audio_struct->rawBuffer; 

	for(unsigned int n=0; n<audio_struct->numOfSamples; n++) 
	{
			int res = byteCombine(sampleBytes, audio_struct); // function pointer, gets sample value by combining the consecutive bytes, organized in either little or big endian
			// if retrieved value is greater than maximum value allowed within current format
			// we have to manually complete the 2's complement 
			if(res>audio_struct->scaleVal)
				res |= audio_struct->mask;
			audio_struct->audioBuffer[n] = res/((float)(audio_struct->scaleVal)); // turn int sample into full scale normalized float

			sampleBytes += audio_struct->bps; // jump to next sample
	}
}
void fromRawToFloat_float32(audio_struct *audio_struct) 
{
	union {
		float f;
		int i;
	} fval;

	unsigned char *sampleBytes = (unsigned char *)audio_struct->rawBuffer; 

	for(unsigned int n=0; n<audio_struct->numOfSamples; n++) 
	{
		int res = byteCombine(sampleBytes, audio_struct); // function pointer, gets sample value by combining the consecutive bytes, organized in either little or big endian

		fval.i = res; // safe
		audio_struct->audioBuffer[n] = fval.f; // interpret as float

		sampleBytes += audio_struct->bps; // jump to next sample
	}
}

//TODO optimize: loop unrolling and, when possible, NEON
//VIC on android, it seems that interleaved is the only way to go
void fromFloatToRaw_int(audio_struct *audio_struct)
{
	unsigned char *sampleBytes = (unsigned char *)audio_struct->rawBuffer; 

	for(unsigned int n=0; n<audio_struct->numOfSamples; n++) 
	{
		int res = audio_struct->scaleVal * audio_struct->audioBuffer[n]; // get actual int sample out of normalized full scale float
		
		byteSplit(sampleBytes, res, audio_struct); // function pointer, splits int into consecutive bytes in either little or big endian

		sampleBytes += audio_struct->bps; // jump to next sample
	}
	// clean up buffer for next period
	memset(audio_struct->audioBuffer, 0, audio_struct->numOfSamples*sizeof(float));
}

void fromFloatToRaw_float32(audio_struct *audio_struct)
{
	union {
		float f;
		int i;
	} fval;

	unsigned char *sampleBytes = (unsigned char *)audio_struct->rawBuffer; 

	for(unsigned int n=0; n<audio_struct->numOfSamples; n++) 
	{
		fval.f = audio_struct->audioBuffer[n]; // safe, cos float is at least 32 bits
		int res = fval.i; // interpret as int
		
		byteSplit(sampleBytes, res, audio_struct); // function pointer, splits int into consecutive bytes in either little or big endian

		sampleBytes += audio_struct->bps; // jump to next sample
	}
	// clean up buffer for next period
	memset(audio_struct->audioBuffer, 0, audio_struct->numOfSamples*sizeof(float));
}


int checkCardFormats(struct pcm_params *params, unsigned int value)
{
	int mismatch = -1;

	printf("\tFormats:\n");
	for(int f=0; f<LDSP_pcm_format::MAX; f++)
	{
		int supported = pcm_params_format_test(params, (pcm_format)f);
		LDSP_pcm_format ff = (LDSP_pcm_format::_enum)LDSP_pcm_format::_from_index(f);
        string name = ff._to_string();		
		if(supported)
			printf("\t\t %s: supported\n", name.c_str());
		else
		{
			printf("\t\t %s: not supported\n", name.c_str());
			if(value == f)
				mismatch = f;
		}
	}

	if(mismatch>=0)
	{
		LDSP_pcm_format ff = (LDSP_pcm_format::_enum)LDSP_pcm_format::_from_index(value);
        string name = ff._to_string();	
		fprintf(stderr, "Device does not support requested format %s!\n", name.c_str());
		return -1;
	}

	return 0;
}
// adapted from here:
// https://github.com/intel/bat/blob/master/bat/tinyalsa.c
int checkCardParam(/*LDSP_*/pcm_params *params, /*LDSP_*/pcm_param param, unsigned int value, string param_name, string param_unit)
{
	unsigned int min;
	unsigned int max;
	int ret = 0;

	// min = LDSP_pcm_params_get_min(params, param);
	// max = LDSP_pcm_params_get_max(params, param);
	min = pcm_params_get_min(params, param);
	max = pcm_params_get_max(params, param);


	printf("\t%s min and max: [%u, %u]\n", param_name.c_str(), min, max);

	if (value < min) {
		fprintf(stderr, "%s is %u %s, device only supports >= %u %s!\n",
			param_name.c_str(), value, param_unit.c_str(), min, param_unit.c_str());
		ret = -2;
	}
	else if (value > max) {
		fprintf(stderr, "%s is %u %s, device only supports <= %u %s!\n",
			param_name.c_str(), value, param_unit.c_str(), max, param_unit.c_str());
		ret = -3;
	}


	return ret;
}

int checkAllCardParams(audio_struct *audioStruct)
{
	//LDSP_pcm_params *params;
	pcm_params *params;
	int err = 0;

	params = /* LDSP_ */pcm_params_get(audioStruct->card, audioStruct->device, audioStruct->flags);
	if (params == NULL) {
		fprintf(stderr, "Unable to open PCM card %u device %u!\n", audioStruct->card, audioStruct->device);
		return -1;
	}


    printf("\nSupported params:\n");

	err = checkCardParam(params, /* LDSP_ */PCM_PARAM_PERIOD_SIZE, audioStruct->config.period_size, "Period size", "Hz");
	if(err==0)
		err = checkCardParam(params, /* LDSP_ */PCM_PARAM_PERIODS, audioStruct->config.period_count, "Period count", "");
	if(err==0)
		err = checkCardParam(params, /* LDSP_ */PCM_PARAM_CHANNELS, audioStruct->config.channels, "Channels", "channels");
	if(err==0)
		err = checkCardParam(params, /* LDSP_ */PCM_PARAM_RATE, audioStruct->config.rate, "Sample rate", "Hz");
	if(err==0)
	 	err = checkCardFormats(params, audioStruct->config.format);

	printf("\n");

	//LDSP_pcm_params_free(params);
	pcm_params_free(params);

	return err;
}






void setGovernorMode() 
{

	unsigned int num_cpus = std::thread::hardware_concurrency();
	governors.resize(num_cpus);

    DIR *dir = opendir(GOVERNOR_BASE_DIR);
    if (dir == nullptr) 
	{
		if(audioVerbose)
        	printf("Could not open directory %s to set performance governor\n", GOVERNOR_BASE_DIR);
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != nullptr)
	{
		if (entry->d_type != DT_DIR)
			continue;

		string dir_name = entry->d_name;

        // Check if directory is named cpuN, where N is an integer between 0 and 9
		if (dir_name.substr(0, 3) == "cpu" && dir_name.length() == 4)		
		{
			int N = stoi(dir_name.substr(3)); // number of current cpu
	
            string cpuDir = string(GOVERNOR_BASE_DIR) + entry->d_name;
            DIR *cpuFreqDir = opendir((cpuDir + "/cpufreq").c_str());
            if (cpuFreqDir != nullptr) 
			{
                // Set scaling_governor to performance if it exists
                string governorPath = cpuDir + "/cpufreq/scaling_governor";
                ifstream governorFile(governorPath);
                if (governorFile.good()) 
				{
					governorFile >> governors[N]; // save current governor
                    governorFile.close();
                    ofstream outGovernorFile(governorPath);
                    outGovernorFile << "performance"; // enable performance governor
                    outGovernorFile.close();
                }
                closedir(cpuFreqDir);
            }
        }
    }

    closedir(dir);
}

void resetGovernorMode()
{
	for (int i = 0; i < governors.size(); i++) 
	{
		// this cpu's geovernor was not modified
		if(governors[i]=="")
			continue;

		string cpuPath = string(GOVERNOR_BASE_DIR) + "/cpu" + std::to_string(i);
		string freqPath = cpuPath + "/cpufreq/scaling_governor";

		// Write to scaling governor file if it exists
		ofstream freqFile(freqPath);
		if (freqFile.good()) 
			freqFile << governors[i];
		freqFile.close();
	}
}




//-----------------------------------------------------------------------------------------------------
// inline

// combine all the bytes [1 or more, according to format] of a sample into an integer
// little endian -> byte_0, byte_1, ..., byte_n-1 [more common format]
inline int byteCombine_littleEndian(unsigned char *sampleBytes, struct audio_struct* audio_struct) 
{
	int value = 0;
	for (int i = 0; i<audio_struct->bps; i++)
		value += (int) (*(sampleBytes + i)) << i * 8;  // combines each sample [which stretches over 1 or more bytes, according to format] in single integer
		
	return value;
}
// little endian -> byte_n-1, byte_n-2, ..., byte_0
inline int byteCombine_bigEndian(unsigned char *sampleBytes, struct audio_struct* audio_struct) {
	int value = 0;
	for (int i = 0; i<audio_struct->bps; i++)
		value += (int) (*(sampleBytes  + audio_struct->physBps - 1 - i)) << i * 8; // combines each sample [which stretches over 1 or more bytes, according to format] in single integer, but reverses the order
	return value;
}

// split an integer sample over more bytes [1 or more, according to format]
// little endian -> byte_0, byte_1, ..., byte_n-1 [more common format]
inline void byteSplit_littleEndian(unsigned char* sampleBytes, int value, struct audio_struct* audio_struct)
{
	for (int i = 0; i <audio_struct->bps; i++)
		*(sampleBytes + i) = (value >>  i * 8) & 0xff;
}
// little endian -> byte_n-1, byte_n-2, ..., byte_0
inline void byteSplit_bigEndian(unsigned char *sampleBytes, int value, struct audio_struct* audio_struct) 
{
	for (int i = 0; i <audio_struct->bps; i++)
		*(sampleBytes + audio_struct->physBps - 1 - i) = (value >> i * 8) & 0xff; 
}
