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
#include <unistd.h> // getpid()
#include <cstdlib> // 
#include <iostream> //system()

#include "LDSP.h"
#include "tinyalsaAudio.h"
#include "mixer.h"
#include "audioDeviceInfo.h"
// #include "tinyAlsaExtension.h"
#include "thread_utils.h"
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

bool perfModeOff = false;
bool sensorsOff_ = false;
bool ctrlInputsOff_ = false;
bool ctrlOutputsOff_ = false;

int cpuIndex = -1;

bool audioServerStopped = false;
#if __ANDROID_API__ > 24
// on Android 7 and above [api 24 and above], Android controls audio via the dedicated audioserver
string audio_server = "audioserver";
#else
// on Android 6 and below [api 23 and below], Android controls audio via the generic media server
string audio_server = "media";
#endif



// Depending on if NEON_AUDIO_FORMAT, fromRawToFloat and fromFloatToRaw will be implenented with NEON or not

// playback
void fromFloatToRaw(LDSPpcmContext *pcmContext);

// capture
void fromRawToFloat(LDSPpcmContext *pcmContext);

// these are inline functions
#ifdef NEON_AUDIO_FORMAT
	// playback
	void byteSplit_littleEndian_neon(unsigned char**, int32x4_t, audio_struct*);
	void byteSplit_bigEndian_neon(unsigned char**, int32x4_t, audio_struct*);
	// capture
	int32x4_t byteCombine_littleEndian_neon(unsigned char**, audio_struct*);
	int32x4_t byteCombine_bigEndian_neon(unsigned char**, audio_struct*);
#else
	// playback
	void byteSplit_littleEndian(unsigned char*, int, audio_struct*);
	void byteSplit_bigEndian(unsigned char*, int, audio_struct*);
	// capture
	int byteCombine_littleEndian(unsigned char*, audio_struct*);
	int byteCombine_bigEndian(unsigned char*, audio_struct*);
#endif


// non-exposed functions
void initAudioParams(LDSPinitSettings *settings, audio_struct **audioStruct, bool is_playback);
void updateAudioParams(LDSPinitSettings *settings, audio_struct **audioStruct, bool is_playback);
void cleanupAudioParams(LDSPpcmContext *pcmContext);
int prepareForFormat(string format, LDSPpcmContext *pcmContext);
int initPcm(audio_struct *audio_struct_p, audio_struct *audio_struct_c);
void cleanupPcm(LDSPpcmContext *pcmContext);
int initLowLevelAudioStruct(audio_struct *audio_struct);
void deallocateLowLevelAudioStruct(audio_struct *audio_struct);
void cleanupLowLevelAudioStruct(LDSPpcmContext *pcmContext);
void setGovernorMode();
void resetGovernorMode();
void *audioLoop(void*); 
void controlAudioserver(int serverState);


int LDSP_initAudio(LDSPinitSettings *settings, void *userData)
{
	if(!settings)
		return -1;
	
	fullDuplex = !settings->captureOff;
	audioVerbose = settings->verbose;
	perfModeOff = settings->perfModeOff;
	sensorsOff_ = settings->sensorsOff;
	ctrlInputsOff_ = settings->ctrlInputsOff;
	ctrlOutputsOff_ = settings->ctrlOutputsOff;
	cpuIndex = settings->cpuIndex;
	

	if(audioVerbose)
		printf("\nLDSP_initAudio()\n");


	if(!settings->keepAudioserver)
		controlAudioserver(0);


	initAudioParams(settings, &pcmContext.playback, true);
	initAudioParams(settings, &pcmContext.capture, false);

	if(prepareForFormat(settings->pcmFormatString, &pcmContext)<0) // format is the same for playback and capture
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

	// once the pcm device is open, we can check if the requested params have been set
	// and update our variables according to the actual params
  	updateAudioParams(settings, &pcmContext.playback, true);
	if(fullDuplex) 
	{
    updateAudioParams(settings, &pcmContext.capture, false);
		if(pcmContext.playback->config.period_size != pcmContext.capture->config.period_size) 
		{
		  fprintf(stderr, "The requested period size results in different sizes for playback (%d) and capture (%d)! Please choose a different one\n", pcmContext.playback->config.period_size, pcmContext.capture->config.period_size);
			return -3;
		}

	}

	if(initLowLevelAudioStruct(pcmContext.playback)<0)
	{
		// try partial deallocation
		deallocateLowLevelAudioStruct(pcmContext.playback);
		return -4;
	}

	if(fullDuplex)
	{
		if(initLowLevelAudioStruct(pcmContext.capture)<0)
		{
			// try partial deallocation
			deallocateLowLevelAudioStruct(pcmContext.capture);
			return -4;
		}
	}

	// activate performance governor
	if(!perfModeOff)
		setGovernorMode();
	
	gFormats.clear();

	// init context
	intContext.projectName = settings->projectName;
	intContext.audioIn = pcmContext.capture->audioBuffer;
	intContext.audioOut = pcmContext.playback->audioBuffer;
	intContext.audioFrames = pcmContext.playback->config.period_size;
	intContext.audioInChannels = pcmContext.capture->config.channels;
	intContext.audioOutChannels = pcmContext.playback->config.channels;
	intContext.audioSampleRate = (float)pcmContext.playback->config.rate;
	userContext = (LDSPcontext*)&intContext;

	return 0;
}

int LDSP_startAudio(void *userData)
{	
	if(audioVerbose)
	{
		printf("\nLDSP_startAudio()\n");
		printf("Starting audio thread...\n");
	}


	if(!setup(userContext, userData))
	{
		LDSP_requestStop();
		return -1;
	}

	pthread_t audioThread;
	if( pthread_create(&audioThread, nullptr, audioLoop, nullptr) ) 
	{
		fprintf(stderr, "Error: unable to create thread\n");
		return -2;
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

	if(!perfModeOff)
		resetGovernorMode();
	
	if(audioServerStopped)
		controlAudioserver(1);
}

void LDSP_requestStop()
{
	gShouldStop = true;
}


//------------------------------------------------------------------------------------
int checkAllCardParams(audio_struct *audioStruct);
int readCardParam(string info_path, string param_match, string &param);

void initAudioParams(LDSPinitSettings *settings, audio_struct **audioStruct, bool is_playback)
{
	string audio_type = "";
	if (is_playback) {
		audio_type = "playback";
	}
	else {
		audio_type = "capture";
	}

	#ifdef NEON_AUDIO_FORMAT
		// ByteAlign audioStruct so that it is prepared for NEON
		if (posix_memalign((void**)*&audioStruct, 16, sizeof(audio_struct)) != 0) {
			printf("Warning! posix_memalign for audio_struct of %s failed\n", audio_type.c_str());
		}
	#else
		*audioStruct = (audio_struct*) malloc(sizeof(audio_struct));
	#endif
	
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
    (*audioStruct)->config.period_count = settings->periodCount;
    (*audioStruct)->config.rate = settings->samplerate;
    (*audioStruct)->config.format = (pcm_format) gFormats[settings->pcmFormatString];
	(*audioStruct)->config.avail_min = 1;
    (*audioStruct)->config.start_threshold = 1;//settings->periodSize; //1; 
    (*audioStruct)->config.stop_threshold = 2 * settings->periodSize * settings->periodCount; //0;
    (*audioStruct)->config.silence_threshold = 0; //1; 
    (*audioStruct)->config.silence_size = 0;



	(*audioStruct)->pcm = nullptr;
	(*audioStruct)->fd = (int)NULL; // needs C's NULL

	if( audioVerbose && 
		( is_playback || (!is_playback && fullDuplex) ) )
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
		printf("\tPeriod size (Audio frames): %d\n", (*audioStruct)->config.period_size);
		printf("\tPeriod count: %d\n", (*audioStruct)->config.period_count);
		printf("\tChannels: %d\n", (*audioStruct)->config.channels);
		printf("\tRate: %d\n", (*audioStruct)->config.rate);
		printf("\tFormat: %s\n", settings->pcmFormatString.c_str());
		printf("\n");
	}
}

void updateAudioParams(LDSPinitSettings *settings, audio_struct **audioStruct, bool is_playback)
{
		string match;
		string value;
		string deviceHwParamsPath = getDeviceInfoPath((*audioStruct)->card, (*audioStruct)->device, "sub0/hw_params", is_playback);		
		
		match = "period_size: ";
		int ret = readCardParam(deviceHwParamsPath, match, value);
		if(ret==0)
			(*audioStruct)->config.period_size = atoi(value.c_str());

		match = "buffer_size: ";
		ret = readCardParam(deviceHwParamsPath, match, value);
		if(ret==0)
			(*audioStruct)->config.period_count = atoi(value.c_str()) / (*audioStruct)->config.period_size;

		match = "channels: ";
		ret = readCardParam(deviceHwParamsPath, match, value);
		if(ret==0)
			(*audioStruct)->config.channels = atoi(value.c_str());
		
		match = "rate: ";
		ret = readCardParam(deviceHwParamsPath, match, value);
		if(ret==0)
			(*audioStruct)->config.rate = atoi(value.c_str());
		
		match = "format: ";
		ret = readCardParam(deviceHwParamsPath, match, value);
		if(ret==0)
			settings->pcmFormatString = value;


		if( audioVerbose && 
			( is_playback || (!is_playback && fullDuplex) ) )
		{
			if(is_playback)
				printf("\nPlayback ");    
			else
				printf("\nCapture ");    
		printf("assigned params:\n");
		printf("\tPeriod size (Audio frames): %d\n", (*audioStruct)->config.period_size);
		printf("\tPeriod count: %d\n", (*audioStruct)->config.period_count);
		printf("\tChannels: %d\n", (*audioStruct)->config.channels);
		printf("\tRate: %d\n", (*audioStruct)->config.rate);
		printf("\tFormat: %s\n", settings->pcmFormatString.c_str());
	}
}

void cleanupAudioParams(LDSPpcmContext *pcmContext)
{
	if(pcmContext->playback != nullptr)
		free(pcmContext->playback);
	if(pcmContext->capture != nullptr)
		free(pcmContext->capture);
}

// this function sets the variables that will drive the format conversion and byteSplit/Combine function calls, including neon case!
int prepareForFormat(string format, LDSPpcmContext *pcmContext)
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
			pcmContext->isBigEndian = false;
			pcmContext->isFloat = false;
			break;
		case LDSP_pcm_format::S16_BE:
		case LDSP_pcm_format::S32_BE:
		case LDSP_pcm_format::S24_BE:
		case LDSP_pcm_format::S24_3BE:
			pcmContext->isBigEndian = true;
			pcmContext->isFloat = false;
			break;
		case LDSP_pcm_format::FLOAT_LE:
			// No support for NEON yet
			pcmContext->isBigEndian = false;
			pcmContext->isFloat = true;
			break;
		case LDSP_pcm_format::FLOAT_BE:
			// No support for NEON yet
			pcmContext->isBigEndian = true;
			pcmContext->isFloat = true;
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
			fprintf(stderr, "Failed to open capture pcm %u,%u – %s\n", audio_struct_c->card, audio_struct_c->device, pcm_get_error(audio_struct_c->pcm));
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

	if(pcm_prepare(audio_struct_p->pcm) < 0)
	{
		fprintf(stderr, "Failed to prepare playback audio device. %s\n", pcm_get_error(audio_struct_p->pcm));
		return -3;
	}

	if(fullDuplex && pcm_prepare(audio_struct_c->pcm) < 0)
	{
		fprintf(stderr, "Failed to prepare capture audio device. %s\n", pcm_get_error(audio_struct_c->pcm));
		return -3;
	}


	audio_struct_p->rawBuffer = nullptr;
	audio_struct_p->audioBuffer = nullptr;
	audio_struct_c->rawBuffer = nullptr;
	audio_struct_c->audioBuffer = nullptr;

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
	int channels = audio_struct->config.channels;
	// to cover the case of capture when non full duplex engine
	if(channels <=0)
		channels = 1;

	// format's bits and max val
	audio_struct->physBps = pcm_format_to_bits(audio_struct->config.format) / 8;  // size in bytes of the format var type used to store sample
	// different than this, i.e., number of bytes actually used within that format var type!
	if(audio_struct->config.format != gFormats["S24_3LE"] && audio_struct->config.format != gFormats["S24_3BE"]) // these vars span 32 bits, but only 24 are actually used! -> 3 bytes
		audio_struct->bps = audio_struct->physBps;
	else
		audio_struct->bps = 3;
	audio_struct->formatBits = audio_struct->bps * 8;
	audio_struct->maxVal = (1 << (audio_struct->formatBits - 1)) - 1;

	// buffer sizes
	audio_struct->numOfSamples = channels*audio_struct->config.period_size;
#ifdef NEON_AUDIO_FORMAT
	// NEON requires a buffer size that is a multiple of either 4  or 8
	// these formats use 16 bits per sample, so we can work on 8 samples in parallel on NEON!
	if(audio_struct->config.format == gFormats["S16_LE"]   || audio_struct->config.format == gFormats["S16_BE"]) 
		audio_struct->numOfSamples = (audio_struct->numOfSamples + 7) & ~7U; // Round up to multiple of 8 for these formats
	// for the other formats, we work on 4 samples in parallel on NEON, regardless on the bits per sample
	else 		
		audio_struct->numOfSamples = (audio_struct->numOfSamples + 3) & ~3U; // Round up to multiple of 4
	unsigned int localFrames   = audio_struct->numOfSamples * audio_struct->physBps; // compute new total number of bytes for frame buffer/raw buffer
#else
	unsigned int localFrames = audio_struct->frameBytes;
#endif
	
	// allocate buffers
	audio_struct->rawBuffer = malloc(localFrames);
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


	// this is used for capture only
	// we compute the mask necessary to complete the two's complement of received raw samples
	// in other words, we will use this to extend the sign of negative numbers that are composed of fewer bits than the int container
	audio_struct->captureMask = 0x00000000;
	// for (int i = 0; i<sizeof(int)-audio_struct->bps; i++)
	// 	audio_struct->captureMask |= (0xFF000000 >> i*8);
	for(int i=audio_struct->formatBits; i < sizeof(int)*8; i++) 
		audio_struct->captureMask |= (1 << i); // put a 1 in all bits that are beyond those used by the format


	// These fields are only defined if NEON is enabled
	#ifdef NEON_AUDIO_FORMAT
		audio_struct->capture_maskVec = vdupq_n_s32(audio_struct->captureMask);
		audio_struct->maxVec = vdupq_n_u32(audio_struct->maxVal);
		audio_struct->factorVec = vdupq_n_f32(audio_struct->maxVal);
		audio_struct->factorVecReciprocal = vdupq_n_f32(1.0 / audio_struct->maxVal);
		audio_struct->byteSplit_maskVec = vdupq_n_s32(0xff);
	#endif
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
	// set the affinity to ensure the thread runs on chosen CPU
	if(cpuIndex > -1)
		set_cpu_affinity(cpuIndex, "audio", audioVerbose);

	// set thread priority
	set_priority(LDSPprioOrder_audio, "audio",audioVerbose);

	set_to_foreground("audio", audioVerbose);

	// set minimum thread niceness
 	set_niceness(-20, "audio",audioVerbose); // only necessary if not real-time, but just in case...


	while(!gShouldStop)
	{
		if(fullDuplex)
		{
			if(pcm_read(pcmContext.capture->pcm, pcmContext.capture->rawBuffer, pcmContext.capture->frameBytes)!=0)
				fprintf(stderr, "\nCapture error, aborting...\n");

			fromRawToFloat(&pcmContext);
		}

		if(!sensorsOff_)
			readSensors();
		if(!ctrlInputsOff_)
			readCtrlInputs();

		render(userContext, 0);
		
		fromFloatToRaw(&pcmContext);

		if(pcm_write(pcmContext.playback->pcm, pcmContext.playback->rawBuffer, pcmContext.playback->frameBytes)!=0)
			fprintf(stderr, "\nPlayback error, aborting...\n");

		if(!ctrlOutputsOff_)
			writeCtrlOutputs();
	}

	if(audioVerbose)
		printf("Audio thread stopped!\n\n");

	return (void *)0;
}

#ifdef NEON_AUDIO_FORMAT
	//---playback format functions---
	void fromFloatToRaw_int_littleEndian_neon(audio_struct *audio_struct)
	{
		unsigned char *sampleBytes = (unsigned char *)audio_struct->rawBuffer; 

		for(unsigned int n=0; n<audio_struct->numOfSamples; n = n + 4) 
		{	
			// Load the four floating-point inputs into a NEON vector
			float32x4_t inputVec = vld1q_f32(&(audio_struct->audioBuffer[n]));

			// Scale the input by scaleVal
			float32x4_t resultVec = vmulq_f32(inputVec, audio_struct->factorVec);

			// Truncates float type to int type
			int32x4_t intValues = vcvtq_s32_f32(resultVec);

			byteSplit_littleEndian_neon(&sampleBytes, intValues, audio_struct);

			sampleBytes += (4 *audio_struct->physBps);
		}
		// clean up buffer for next period
		memset(audio_struct->audioBuffer, 0, audio_struct->numOfSamples*sizeof(float));
	}

	void fromFloatToRaw_int_bigEndian_neon(audio_struct *audio_struct)
	{
		unsigned char *sampleBytes = (unsigned char *)audio_struct->rawBuffer; 

		for(unsigned int n=0; n<audio_struct->numOfSamples; n = n + 4) 
		{	
			// Load the four floating-point inputs into a NEON vector
			float32x4_t inputVec = vld1q_f32(&(audio_struct->audioBuffer[n]));

			// Scale the input by scaleVal
			float32x4_t resultVec = vmulq_f32(inputVec, audio_struct->factorVec);

			// Truncates float type to int type
			int32x4_t intValues = vcvtq_s32_f32(resultVec);

			byteSplit_bigEndian_neon(&sampleBytes, intValues, audio_struct);

			sampleBytes += (4 *audio_struct->physBps);
		}
		// clean up buffer for next period
		memset(audio_struct->audioBuffer, 0, audio_struct->numOfSamples*sizeof(float));
	}

	// ** This has yet to be tested since we do not have a phone that supports float samples
	void fromFloatToRaw_float_littleEndian_neon(audio_struct *audio_struct)
	{
		float32_t *src = (float32_t*)audio_struct->audioBuffer;
		float32_t *dst = (float32_t*)audio_struct->rawBuffer;
		
		for(unsigned int n = 0; n < audio_struct->numOfSamples; n += 4)
		{
			// Load the four floating-point inputs into a NEON vector
			float32x4_t inputVec = vld1q_f32(&src[n]);
			
			#ifdef __BIG_ENDIAN__
			// CPU is big-endian, format is little-endian - need swap
			uint8x16_t bytes = vreinterpretq_u8_f32(inputVec);
			bytes = vrev32q_u8(bytes);  // Reverse bytes in each 32-bit word
			inputVec = vreinterpretq_f32_u8(bytes);
			#endif
			// If CPU is little-endian, no swap needed
			
			vst1q_f32(&dst[n], inputVec);
		}
		
		// clean up buffer for next period
		memset(audio_struct->audioBuffer, 0, audio_struct->numOfSamples * sizeof(float));
	}

	// ** This has yet to be tested since we do not have a phone that supports float samples
	void fromFloatToRaw_float_bigEndian_neon(audio_struct *audio_struct)
	{
		float32_t *src = (float32_t*)audio_struct->audioBuffer;
		float32_t *dst = (float32_t*)audio_struct->rawBuffer;
		
		for(unsigned int n = 0; n < audio_struct->numOfSamples; n += 4)
		{
			// Load the four floating-point inputs into a NEON vector
			float32x4_t inputVec = vld1q_f32(&src[n]);
			
			#ifndef __BIG_ENDIAN__
			// CPU is little-endian, format is big-endian - need swap
			uint8x16_t bytes = vreinterpretq_u8_f32(inputVec);
			bytes = vrev32q_u8(bytes);  // Reverse bytes in each 32-bit word
			inputVec = vreinterpretq_f32_u8(bytes);
			#endif
			// If CPU is big-endian, no swap needed
			
			vst1q_f32(&dst[n], inputVec);
		}
		
		// clean up buffer for next period
		memset(audio_struct->audioBuffer, 0, audio_struct->numOfSamples * sizeof(float));
	}

	// NEON implementation of fromFloatToRaw
	//VIC on android, it seems that interleaved is the only way to go
	void fromFloatToRaw(LDSPpcmContext *pcmContext)
	{
		audio_struct *audio_struct = pcmContext->playback;
		
		// most common formats! faster specific implementation
		if(!pcmContext->isFloat && audio_struct->bps == 2)
		{
			int16_t *dst = (int16_t*)audio_struct->rawBuffer;
			
			// Optimized path: 16-bit integer, little endian and big endian
			// Process all samples - guaranteed to be multiple of 8
			for(unsigned int n = 0; n < audio_struct->numOfSamples; n += 8)
			{
				// Load 8 floats
				float32x4_t vec1 = vld1q_f32(&audio_struct->audioBuffer[n]);
				float32x4_t vec2 = vld1q_f32(&audio_struct->audioBuffer[n + 4]);
				
				// Scale by maxVal
				vec1 = vmulq_f32(vec1, audio_struct->factorVec);
				vec2 = vmulq_f32(vec2, audio_struct->factorVec);
				
				// Convert to int32
				int32x4_t i32_vec1 = vcvtq_s32_f32(vec1);
				int32x4_t i32_vec2 = vcvtq_s32_f32(vec2);
				
				// Convert to int16 and combine
				int16x4_t i16_vec1 = vmovn_s32(i32_vec1);
				int16x4_t i16_vec2 = vmovn_s32(i32_vec2);
				int16x8_t combined = vcombine_s16(i16_vec1, i16_vec2);
				
				// Handle endianness
				if(!pcmContext->isBigEndian)
				{
					#ifdef __BIG_ENDIAN__
					// Swap bytes if CPU is big-endian but format is little
					combined = vreinterpretq_s16_u8(vrev16q_u8(vreinterpretq_u8_s16(combined)));
					#endif
				}
				else
				{
					#ifndef __BIG_ENDIAN__
					// Swap bytes if CPU is little-endian but format is big
					combined = vreinterpretq_s16_u8(vrev16q_u8(vreinterpretq_u8_s16(combined)));
					#endif
				}
				
				// Store 8 samples at once
				vst1q_s16(dst, combined);
				dst += 8;
			}
			
			memset(audio_struct->audioBuffer, 0, audio_struct->numOfSamples * sizeof(float));
		}
		// other integer formats, less common, slower generic implemenations
		else if(!pcmContext->isFloat)
		{
			if(!pcmContext->isBigEndian)
				fromFloatToRaw_int_littleEndian_neon(audio_struct);
			else
				fromFloatToRaw_int_bigEndian_neon(audio_struct);
		}
		// float formats, least common but easy fast specific implementations		
		else
		{
			if(!pcmContext->isBigEndian)
				fromFloatToRaw_float_littleEndian_neon(audio_struct);
			else
				fromFloatToRaw_float_bigEndian_neon(audio_struct);
		}
	}


	//---capture format functions---
	void fromRawToFloat_int_littleEndian_neon(audio_struct *audio_struct)
	{
		unsigned char *sampleBytes = (unsigned char *)audio_struct->rawBuffer; 

		for(unsigned int n=0; n<audio_struct->numOfSamples; n = n + 4) 
		{
				int32x4_t res = byteCombine_littleEndian_neon(&sampleBytes, audio_struct);

				sampleBytes += (4 *audio_struct->physBps);

				// Perform the comparison (res > scaleVal)
				// We are comparing 4 signed integers (res) with 4 unsigned integers (scaleVec) to identify negative
				// numbers, and apply sign extension with the captureMask
				// Note: If we are using a 32 bit format, there is no need for sign extension, but negative numbers
				//       will still return 1 in this comparison, triggering the masking with capture_maskVec,
				// BUT in that case, the capture_maskVec is composed of 0's so, doing a logical OR with it will not matter
				uint32x4_t greaterThanMask = vcgtq_u32(res, audio_struct->maxVec);

				// Prepare the mask by ANDing it with the captureMask
				int32x4_t maskedValues = vandq_s32((int32x4_t)greaterThanMask, audio_struct->capture_maskVec);

				// Apply the mask using OR operation
				int32x4_t masked_result = vorrq_s32(res, maskedValues);

				// Convert masked_result to float type
				float32x4_t floatVec = vcvtq_f32_s32(masked_result);

				// Scale integer representation to float representation
				float32x4_t result = vmulq_f32(floatVec, audio_struct->factorVecReciprocal);

				audio_struct->audioBuffer[n] = result[0];
				audio_struct->audioBuffer[n + 1] = result[1];
				audio_struct->audioBuffer[n + 2] = result[2];
				audio_struct->audioBuffer[n + 3] = result[3];		
		}
	}

	void fromRawToFloat_int_bigEndian_neon(audio_struct *audio_struct)
	{
		unsigned char *sampleBytes = (unsigned char *)audio_struct->rawBuffer; 

		for(unsigned int n=0; n<audio_struct->numOfSamples; n = n + 4) 
		{
				int32x4_t res = byteCombine_bigEndian_neon(&sampleBytes, audio_struct);

				sampleBytes += (4 *audio_struct->physBps);

				// Perform the comparison (res > scaleVal)
				// We are comparing 4 signed integers (res) with 4 unsigned integers (scaleVec) to identify negative
				// numbers, and apply sign extension with the captureMask
				// Note: If we are using a 32 bit format, there is no need for sign extension, but negative numbers
				//       will still return 1 in this comparison, triggering the masking with capture_maskVec,
				// BUT in that case, the capture_maskVec is composed of 0's so, doing a logical OR with it will not matter
				uint32x4_t greaterThanMask = vcgtq_u32(res, audio_struct->maxVec);

				// Prepare the mask by ANDing it with the captureMask
				int32x4_t maskedValues = vandq_s32((int32x4_t)greaterThanMask, audio_struct->capture_maskVec);

				// Apply the mask using OR operation
				int32x4_t masked_result = vorrq_s32(res, maskedValues);

				// Convert masked_result to float type
				float32x4_t floatVec = vcvtq_f32_s32(masked_result);

				// Scale integer representation to float representation
				float32x4_t result = vmulq_f32(floatVec, audio_struct->factorVecReciprocal);

				audio_struct->audioBuffer[n] = result[0];
				audio_struct->audioBuffer[n + 1] = result[1];
				audio_struct->audioBuffer[n + 2] = result[2];
				audio_struct->audioBuffer[n + 3] = result[3];		
		}
	}

	// ** This has yet to be tested since we do not have a phone that supports float samples
	void fromRawToFloat_float_littleEndian_neon(audio_struct *audio_struct) 
	{
		float32_t *src = (float32_t*)audio_struct->audioBuffer;
		float32_t *dst = (float32_t*)audio_struct->rawBuffer;
		
		for(unsigned int n = 0; n < audio_struct->numOfSamples; n += 4)
		{
			// Load the four floating-point inputs into a NEON vector
			float32x4_t inputVec = vld1q_f32(&src[n]);
			
			#ifdef __BIG_ENDIAN__
			// CPU is big-endian, format needs to be little-endian - swap
			uint8x16_t bytes = vreinterpretq_u8_f32(inputVec);
			bytes = vrev32q_u8(bytes);  // Reverse bytes in each 32-bit word
			inputVec = vreinterpretq_f32_u8(bytes);
			#endif
			// If CPU is little-endian, no swap needed (native = output format)
			
			vst1q_f32(&dst[n], inputVec);
		}
		
		// clean up buffer for next period
		memset(audio_struct->audioBuffer, 0, audio_struct->numOfSamples * sizeof(float));
	}

	// ** This has yet to be tested since we do not have a phone that supports float samples
	void fromRawToFloat_float_bigEndian_neon(audio_struct *audio_struct) 
	{
		float32_t *src = (float32_t*)audio_struct->audioBuffer;
		float32_t *dst = (float32_t*)audio_struct->rawBuffer;
		
		for(unsigned int n = 0; n < audio_struct->numOfSamples; n += 4)
		{
			// Load the four floating-point inputs into a NEON vector
			float32x4_t inputVec = vld1q_f32(&src[n]);
			
#ifndef __BIG_ENDIAN__
			// CPU is little-endian, format needs to be big-endian - swap
			uint8x16_t bytes = vreinterpretq_u8_f32(inputVec);
			bytes = vrev32q_u8(bytes);  // Reverse bytes in each 32-bit word  
			inputVec = vreinterpretq_f32_u8(bytes);
#endif
			// If CPU is big-endian, no swap needed (native = output format)
			
			vst1q_f32(&dst[n], inputVec);
		}
		
		// clean up buffer for next period
		memset(audio_struct->audioBuffer, 0, audio_struct->numOfSamples * sizeof(float));
	}

	// NEON implementation of fromRawToFloat
	//VIC on android, it seems that interleaved is the only way to go
	void fromRawToFloat(LDSPpcmContext *pcmContext)
	{
		audio_struct *audio_struct = pcmContext->capture;
		
		// most common formats! faster specific implementation
		if(!pcmContext->isFloat && audio_struct->bps == 2)
		{
			uint16_t *src = (uint16_t*)audio_struct->rawBuffer;
			float32_t *dst = audio_struct->audioBuffer;
			
			// Optimized path: 16-bit integer, little endian and big endian
			// Process all samples - guaranteed to be multiple of 8
			for(unsigned int n = 0; n < audio_struct->numOfSamples; n += 8)
			{
				// Load 8 int16 samples
				int16x8_t i16_vec = vld1q_s16((int16_t*)&src[n]);
				
				// Handle endianness
				if(!pcmContext->isBigEndian)
				{
#ifdef __BIG_ENDIAN__
					// CPU is big-endian, format is little-endian - swap
					i16_vec = vreinterpretq_s16_u8(vrev16q_u8(vreinterpretq_u8_s16(i16_vec)));
#endif
				}
				else
				{
#ifndef __BIG_ENDIAN__
					// CPU is little-endian, format is big-endian - swap
					i16_vec = vreinterpretq_s16_u8(vrev16q_u8(vreinterpretq_u8_s16(i16_vec)));
#endif
				}
				
				// Convert to unsigned (matching your byteCombine behavior)
				uint16x8_t u16_vec = vreinterpretq_u16_s16(i16_vec);
				
				// Split into two sets of 4 for conversion to int32
				uint16x4_t u16_low = vget_low_u16(u16_vec);
				uint16x4_t u16_high = vget_high_u16(u16_vec);
				
				// Widen to int32
				uint32x4_t u32_vec1 = vmovl_u16(u16_low);
				uint32x4_t u32_vec2 = vmovl_u16(u16_high);
				
				// Convert to float
				float32x4_t f32_vec1 = vcvtq_f32_u32(u32_vec1);
				float32x4_t f32_vec2 = vcvtq_f32_u32(u32_vec2);
				
				// Scale by dividing by maxVal (or multiply by reciprocal if available)
				f32_vec1 = vmulq_f32(f32_vec1, audio_struct->factorVecReciprocal);
				f32_vec2 = vmulq_f32(f32_vec2, audio_struct->factorVecReciprocal);
				
				// Store 8 floats
				vst1q_f32(&dst[n], f32_vec1);
				vst1q_f32(&dst[n + 4], f32_vec2);
			}
		}
    	// less common formats, slower generic implementations
		// int, little or big endian
		else if(!pcmContext->isFloat)
		{
			if(!pcmContext->isBigEndian)
				fromRawToFloat_int_littleEndian_neon(audio_struct);
			else
				fromRawToFloat_int_bigEndian_neon(audio_struct);
		}
		// float formats, least common but easy fast specific implementations
		else
		{
			if(!pcmContext->isBigEndian)
				fromRawToFloat_float_littleEndian_neon(audio_struct);
			else
				fromRawToFloat_float_bigEndian_neon(audio_struct);
		}
	}
#else
	//---playback format functions---
	void fromFloatToRaw_int_litteEndian(audio_struct *audio_struct)
	{
		unsigned char *sampleBytes = (unsigned char *)audio_struct->rawBuffer; 
		for(unsigned int n=0; n<audio_struct->numOfSamples; n++) 
		{
			int res = audio_struct->maxVal * audio_struct->audioBuffer[n]; // get actual int sample out of normalized full scale float
			
			byteSplit_littleEndian(sampleBytes, res, audio_struct);

			sampleBytes += audio_struct->bps; // jump to next sample
		}
		// clean up buffer for next period
		memset(audio_struct->audioBuffer, 0, audio_struct->numOfSamples*sizeof(float));
	}

	void fromFloatToRaw_int_bigEndian(audio_struct *audio_struct)
	{
		unsigned char *sampleBytes = (unsigned char *)audio_struct->rawBuffer; 
		for(unsigned int n=0; n<audio_struct->numOfSamples; n++) 
		{
			int res = audio_struct->maxVal * audio_struct->audioBuffer[n]; // get actual int sample out of normalized full scale float
			
			byteSplit_bigEndian(sampleBytes, res, audio_struct);

			sampleBytes += audio_struct->bps; // jump to next sample
		}
		// clean up buffer for next period
		memset(audio_struct->audioBuffer, 0, audio_struct->numOfSamples*sizeof(float));
	}
	
	// ** This has yet to be tested since we do not have a bigEndian phone
	void fromFloatToRaw_float_littleEndian(audio_struct *audio_struct)
	{
#ifdef __BIG_ENDIAN__
		// On big-endian system, need to swap bytes
		uint32_t *dst = (uint32_t*)audio_struct->rawBuffer;
		union {
			float f;
			uint32_t i;
		} fval;
		
		for(unsigned int n = 0; n < audio_struct->numOfSamples; n++)
		{
			fval.f = audio_struct->audioBuffer[n];
			*dst++ = __builtin_bswap32(fval.i);  // Swap 4 bytes for little-endian output
		}
#else
		// On little-endian system, writing little-ending floats - direct copy
		memcpy(audio_struct->rawBuffer, audio_struct->audioBuffer, audio_struct->numOfSamples * sizeof(float));
#endif
		
		// clean up buffer for next period
		memset(audio_struct->audioBuffer, 0, audio_struct->numOfSamples * sizeof(float));
	}

	// ** This has yet to be tested since we do not have a bigEndian phone
	void fromFloatToRaw_float_bigEndian(audio_struct *audio_struct)
	{
#ifdef __BIG_ENDIAN__
		// On big-endian system, direct copy (already big-endian)
		memcpy(audio_struct->rawBuffer, audio_struct->audioBuffer, audio_struct->numOfSamples * sizeof(float));
#else
		// On little-endian system (x86/ARM), need to swap bytes
		uint32_t *dst = (uint32_t*)audio_struct->rawBuffer;
		union {
			float f;
			uint32_t i;
		} fval;
		
		for(unsigned int n = 0; n < audio_struct->numOfSamples; n++)
		{
			fval.f = audio_struct->audioBuffer[n];
			*dst++ = __builtin_bswap32(fval.i);  // Swap to big-endian
		}
#endif
		
		// clean up buffer for next period
		memset(audio_struct->audioBuffer, 0, audio_struct->numOfSamples * sizeof(float));
	}

	//TODO:
	//_drop BE arch support and then revise, simplify all conversions [24 and 32 can use bswap32! the only case thta needs manual swap is 24_3]
	//_handle asymmetric quantization on all playback and capture format conversions, including neon!
	//VIC on android, it seems that interleaved is the only way to go
	void fromFloatToRaw(LDSPpcmContext *pcmContext)
	{
		audio_struct *audio_struct = pcmContext->playback;

		// most common formats! faster specific implementation
		if(!pcmContext->isFloat && audio_struct->bps == 2)
		{	
			unsigned char *rawBuffer = (unsigned char*)(audio_struct->rawBuffer);		
			int16_t *dst = (int16_t*)rawBuffer;

			// Optimized path: 16-bit integer, little endian (most common)
			if(!pcmContext->isBigEndian) 
			{
				for(unsigned int n = 0; n < audio_struct->numOfSamples; n++) 
				{
					uint16_t val = (uint16_t)(audio_struct->maxVal * audio_struct->audioBuffer[n]);
#ifdef __BIG_ENDIAN__
					*dst++ = __builtin_bswap16(val);  // Byte swap if running on big-endian CPUs
#else  
					*dst++ = val;  // Direct write if running on little-endian CPUs
#endif
				}
					
			}
			// Optimized path: 16-bit integer, big endian
			else if(!pcmContext->isFloat && pcmContext->isBigEndian && audio_struct->bps == 2)
			{
				for(unsigned int n = 0; n < audio_struct->numOfSamples; n++) 
				{
					uint16_t val = (uint16_t)(audio_struct->maxVal * audio_struct->audioBuffer[n]);
#ifdef __BIG_ENDIAN__
					*dst++ = val;  // Direct write if running on big-endian CPUs
#else  
					*dst++ = __builtin_bswap16(val);  // Byte swap if running on little-endian CPUs
#endif
				}
			}

			// Clean up buffer for next period
			memset(audio_struct->audioBuffer, 0, audio_struct->numOfSamples * sizeof(float));
		}
		// other integer formats, less common, slower generic implementations
		else if(!pcmContext->isFloat)  
		{
			if(!pcmContext->isBigEndian)
				fromFloatToRaw_int_litteEndian(audio_struct);
			else
				fromFloatToRaw_int_bigEndian(audio_struct);
		}	 
		// float formats, least common but easy fast specific implementations
		else 
		{
			if(!pcmContext->isBigEndian)
				fromFloatToRaw_float_littleEndian(audio_struct);
			else
				fromFloatToRaw_float_bigEndian(audio_struct);
		}
	}

	
	//---capture format functions---
	void fromRawToFloat_int_littleEndian(audio_struct *audio_struct) 
	{
		unsigned char *sampleBytes = (unsigned char *)audio_struct->rawBuffer; 

		for(unsigned int n=0; n<audio_struct->numOfSamples; n++) 
		{
			int res = byteCombine_littleEndian(sampleBytes, audio_struct);
			// if retrieved value is greater than maximum value allowed within current format
			// we have to manually complete the 2's complement, by extending the sign
			if(res>audio_struct->maxVal)
				res |= audio_struct->captureMask;
			audio_struct->audioBuffer[n] = res/((float)(audio_struct->maxVal)); // turn int sample into full scale normalized float

			sampleBytes += audio_struct->bps; // jump to next sample
		}
	}

	void fromRawToFloat_int_bigEndian(audio_struct *audio_struct) 
	{
		unsigned char *sampleBytes = (unsigned char *)audio_struct->rawBuffer; 

		for(unsigned int n=0; n<audio_struct->numOfSamples; n++) 
		{
			int res = byteCombine_littleEndian(sampleBytes, audio_struct);
			// if retrieved value is greater than maximum value allowed within current format
			// we have to manually complete the 2's complement, by extending the sign
			if(res>audio_struct->maxVal)
				res |= audio_struct->captureMask;
			audio_struct->audioBuffer[n] = res/((float)(audio_struct->maxVal)); // turn int sample into full scale normalized float

			sampleBytes += audio_struct->bps; // jump to next sample
		}
	}

	// ** This has yet to be tested since we do not have a bigEndian phone
	void fromRawToFloat_float_littleEndian(audio_struct *audio_struct)
	{
#ifdef __BIG_ENDIAN__
		// On big-endian CPU, reading little-endian floats - need to swap
		uint32_t *src = (uint32_t*)audio_struct->rawBuffer;
		union {
			float f;
			uint32_t i;
		} fval;
		
		for(unsigned int n = 0; n < audio_struct->numOfSamples; n++)
		{
			fval.i = __builtin_bswap32(*src++);  // Swap from little-endian
			audio_struct->audioBuffer[n] = fval.f;
		}
#else
		// On little-endian CPU, reading little-endian floats - direct copy
		memcpy(audio_struct->audioBuffer, audio_struct->rawBuffer, audio_struct->numOfSamples * sizeof(float));
#endif
	}

	// ** This has yet to be tested since we do not have a bigEndian phone
	void fromRawToFloat_float_bigEndian(audio_struct *audio_struct)
	{
#ifdef __BIG_ENDIAN__
		// On big-endian CPU, reading big-endian floats - direct copy
		memcpy(audio_struct->audioBuffer, audio_struct->rawBuffer, audio_struct->numOfSamples * sizeof(float));
#else
		// On little-endian CPU, reading big-endian floats - need to swap
		uint32_t *src = (uint32_t*)audio_struct->rawBuffer;
		union {
			float f;
			uint32_t i;
		} fval;
		
		for(unsigned int n = 0; n < audio_struct->numOfSamples; n++)
		{
			fval.i = __builtin_bswap32(*src++);  // Swap from big-endian
			audio_struct->audioBuffer[n] = fval.f;
		}
#endif
	}

	//VIC on android, it seems that interleaved is the only way to go
	void fromRawToFloat(LDSPpcmContext *pcmContext)
	{
		audio_struct *audio_struct = pcmContext->capture;
		
		// most common formats! faster specific implementation
		if(!pcmContext->isFloat && audio_struct->bps == 2)
		{
			unsigned char *rawBuffer = (unsigned char*)(audio_struct->rawBuffer);
			uint16_t *src = (uint16_t*)rawBuffer;

			// Optimized path: 16-bit integer, little endian (most common)
			if(!pcmContext->isBigEndian)
			{
				for(unsigned int n = 0; n < audio_struct->numOfSamples; n++)
				{
					uint16_t val = *src++;
#ifdef __BIG_ENDIAN__
					val = __builtin_bswap16(val);  // Byte swap if running on big-endian CPUs
#endif
					int res = (int)val;
					audio_struct->audioBuffer[n] = (float)res / audio_struct->maxVal; // Direct write if running on little-endian CPUs
				}
			}
			// Optimized path: 16-bit integer, big endian
			else
			{
				for(unsigned int n = 0; n < audio_struct->numOfSamples; n++)
				{
					uint16_t val = *src++;
#ifndef __BIG_ENDIAN__
					val = __builtin_bswap16(val);  // Byte swap if running on little-endian CPUs
#endif
					int res = (int)val;
					audio_struct->audioBuffer[n] = (float)res / audio_struct->maxVal;
				}
			}
		}
    	// other integer formats, less common, slower generic implementations
		else if(!pcmContext->isFloat)
		{
			if(!pcmContext->isBigEndian)
				fromRawToFloat_int_littleEndian(audio_struct);
			else
				fromRawToFloat_int_bigEndian(audio_struct);
		}
		// float formats, least common but easy fast specific implementations
		else
		{
			if(!pcmContext->isBigEndian)
				fromRawToFloat_float_littleEndian(audio_struct);
			else
				fromRawToFloat_float_bigEndian(audio_struct);
		}
	}
#endif






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



int readCardParam(string info_path, string param_match, string &param)
{
	int ret = getDeviceInfo(info_path, param_match, param);
	
	if(ret==-1)
	{
		printf("Warning! Could not open \"%s\" to retrieve device's \"%s\"\n", info_path.c_str(), param_match.c_str());
		return ret;
	}
	else if(ret==-2)
	{
		printf("Warning! Could not find \"%s\" in \"%s\"\n", param_match.c_str(), info_path.c_str());
		return ret;
	}

	trimDeviceInfo(param); // some params may have extra info separated by space
	return 0;
}


// adapted from here:
// https://github.com/intel/bat/blob/master/bat/tinyalsa.c
int checkCardParam(pcm_params *params, pcm_param param, unsigned int value, string param_name, string param_unit)
{
	unsigned int min;
	unsigned int max;
	int ret = 0;

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
	pcm_params *params;
	int err = 0;

	params = /* LDSP_ */pcm_params_get(audioStruct->card, audioStruct->device, audioStruct->flags);
	if (params == NULL) {
		fprintf(stderr, "Unable to open PCM card %u device %u!\n", audioStruct->card, audioStruct->device);
		return -1;
	}


	printf("\nSupported params:\n");

	err = checkCardParam(params, PCM_PARAM_PERIOD_SIZE, audioStruct->config.period_size, "Period size", "Hz");
	if(err==0)
		err = checkCardParam(params, PCM_PARAM_PERIODS, audioStruct->config.period_count, "Period count", "");
	if(err==0)
		err = checkCardParam(params, PCM_PARAM_CHANNELS, audioStruct->config.channels, "Channels", "channels");
	if(err==0)
		err = checkCardParam(params, PCM_PARAM_RATE, audioStruct->config.rate, "Sample rate", "Hz");
	if(err==0)
	 	err = checkCardFormats(params, audioStruct->config.format);

	printf("\n");

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
				// get current scaling_governor if it exists
				string governorPath = cpuDir + "/cpufreq/scaling_governor";
				ifstream governorFile(governorPath);
				if (governorFile.good()) 
				{
					governorFile >> governors[N]; // save current governor
					governorFile.close();
				}
				closedir(cpuFreqDir);
			}
		}
	}

	closedir(dir);


	// repeat to set scaling governors to performances
	// we need two loops, otherwise the modification of a governor of a cpu
	// may change the governor of other cpus
	dir = opendir(GOVERNOR_BASE_DIR);
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
	// inreverse order, cos changing geovernor of first cpu may affect other cpus' governors
	for (int i=governors.size()-1; i>=0; i--) 
	{
		// this cpu's geovernor was not modified
		if(governors[i]=="")
			continue;

		string cpuPath = string(GOVERNOR_BASE_DIR) + "cpu" + std::to_string(i);
		string freqPath = cpuPath + "/cpufreq/scaling_governor";

		// Write to scaling governor file if it exists
		ofstream freqFile(freqPath);
		if (freqFile.good()) 
			freqFile << governors[i]; // always use governor of first cpu, because in most cases cpu0 controls all cpus
	
		freqFile.close();
	}
}



//-----------------------------------------------------------------------------------------------------
// inline

#ifdef NEON_AUDIO_FORMAT
	inline void byteSplit_littleEndian_neon(unsigned char** sampleBytes, int32x4_t values, struct audio_struct* audio_struct)
	{
		int bps = audio_struct->bps;
		for (int i=0; i<bps; i++) 
		{
			int32x4_t shiftAmountVec = vdupq_n_s32(-i*8); // Cannot be created elsewhere because it uses i
			int32x4_t shifted_values = vshlq_s32(values, shiftAmountVec);

			int32x4_t res = vandq_s32(shifted_values, audio_struct->byteSplit_maskVec);

			*(*sampleBytes + i) = res[0];
			*(*sampleBytes + 1 * bps + i) = res[1];
			*(*sampleBytes + 2 * bps + i) = res[2];
			*(*sampleBytes + 3 * bps + i) = res[3];
		}	
	}

	inline void byteSplit_bigEndian_neon(unsigned char **sampleBytes, int32x4_t values, struct audio_struct* audio_struct) 
	{
		int bps = audio_struct->bps;
		for (int i=0; i<bps; i++) 
		{
			// NEON Right shift requires int to be a compile-time constant, so need to left shift by negative
			int32x4_t shiftAmountVec = vdupq_n_s32(-i*8);
			int32x4_t shifted_values = vshlq_s32(values, shiftAmountVec);

			int res[4];

			int32x4_t maskedValues = vandq_s32(shifted_values, audio_struct->byteSplit_maskVec);

			vst1q_s32(res, maskedValues);

			int physBps = audio_struct->physBps;
			*(*sampleBytes + physBps - 1 - i) = res[0];
			*(*sampleBytes + physBps - 1 + 1 * bps - i) = res[1];
			*(*sampleBytes + physBps - 1 + 2 * bps - i) = res[2];
			*(*sampleBytes + physBps - 1 + 3 * bps - i) = res[3];
		}
	}

	int32x4_t byteCombine_littleEndian_neon(unsigned char** sampleBytes, audio_struct* audio_struct)
	{
		int32x4_t values = vdupq_n_s32(0);

		int32x4_t rawValues;
		
		int physBps = audio_struct->physBps;
		for (int i=0; i<physBps; i++) 
		{
			rawValues[0] = *(*sampleBytes + i);
			rawValues[1] = *(*sampleBytes + 1 * physBps + i);
			rawValues[2] = *(*sampleBytes + 2 * physBps + i);
			rawValues[3] = *(*sampleBytes + 3 * physBps + i);

			int32x4_t shiftAmountVec = vdupq_n_s32(i*8);
			int32x4_t shiftedValues = vshlq_s32(rawValues, shiftAmountVec);

			values = vaddq_s32(values, shiftedValues);
		}
		return values;
	}

	int32x4_t byteCombine_bigEndian_neon(unsigned char** sampleBytes, audio_struct* audio_struct)
	{
		int32x4_t values = vdupq_n_s32(0);

		int32x4_t rawValues;
		int bps = audio_struct->bps;
		int physBps = audio_struct->physBps;
		for(int i=0; i<audio_struct->bps; i++) 
		{			
			// Above gets simplified to this:
			rawValues[0] = *(*sampleBytes + 1 * physBps - 1 - i);
			rawValues[1] = *(*sampleBytes + 2 * physBps - 1 - i);
			rawValues[2] = *(*sampleBytes + 3 * physBps - 1 - i);
			rawValues[3] = *(*sampleBytes + 4 * physBps - 1 - i);

			int32x4_t shiftAmountVec = vdupq_n_s32(i*8);
			int32x4_t shiftedValues = vshlq_s32(rawValues, shiftAmountVec);

			values = vaddq_s32(values, shiftedValues);
		}
		return values;
	}

#else
	// combine all the bytes [1 or more, according to format] of a sample into an integer
	// little endian -> byte_0, byte_1, ..., byte_n-1 [more common format]
	inline int byteCombine_littleEndian(unsigned char *sampleBytes, struct audio_struct* audio_struct) 
	{
		int value = 0;
		for (int i = 0; i<audio_struct->bps; i++)
			value += (int) (*(sampleBytes + i)) << i * 8;  // combines each sample [which stretches over 1 or more bytes, according to format] in single integer
			
		return value;
	}
	// big endian -> byte_n-1, byte_n-2, ..., byte_0
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
	// big endian -> byte_n-1, byte_n-2, ..., byte_0
	inline void byteSplit_bigEndian(unsigned char *sampleBytes, int value, struct audio_struct* audio_struct) 
	{
		for (int i = 0; i <audio_struct->bps; i++)
			*(sampleBytes + audio_struct->physBps - 1 - i) = (value >> i * 8) & 0xff; 
	}
#endif


void controlAudioserver(int serverState) 
{
	if(serverState == 0) 
	{
		if(audioVerbose)
			printf("\nTrying to stop Android audio server...\n");
		string server_cmd = "stop "+audio_server;
		if(system(server_cmd.c_str()) != 0)
			printf("\tFailed to stop Android audio server (this should not be an issue)\n");
		else 
		{
			audioServerStopped = true;
			if(audioVerbose) 
				printf("\tAndroid audio server stopped!\n\n");
		}
	}
	else if(audioServerStopped) // && serverState == 1 or else
	{
		if(audioVerbose)
			printf("\nTrying to restart Android audio server...\n");
		string server_cmd = "start "+audio_server;
		if(system(server_cmd.c_str()) != 0)
			printf("\tFailed to restart Android audio server (this should not be an issue)\n");
		else
		{ 
			audioServerStopped = false;
			if(audioVerbose)
				printf("\tAndroid audio server restarted!\n\n");
		}
	
	}
	else if(audioVerbose)
		printf("\nRequest to restart the Android audio server ignored, because already running\n\n");
}