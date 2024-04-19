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
#include <stdlib.h>

#include "LDSP.h"
#include "tinyalsaAudio.h"
#include "mixer.h"
// /#include "tinyAlsaExtension.h"
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

// NEON byteSplit
void byteSplit_littleEndian_NEON(unsigned char**, int32x4_t, audio_struct*);
void byteSplit_littleEndian_unrolled(unsigned char**, int[4], audio_struct*);
void byteSplit_bigEndian_NEON(unsigned char**, int32x4_t, audio_struct*);

void (*fromFloatToRaw)(audio_struct*);
void fromFloatToRaw_int(audio_struct*);
void fromFloatToRaw_float32(audio_struct*);
void fromFloatToRaw_int_NEON(audio_struct*);


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
    
	// Copying values from settings over for ease of reading, and better naming
	fullDuplex = !settings->outputOnly;
    audioVerbose = settings->verbose;
	perfModeOff = settings->perfModeOff;
	sensorsOff_ = settings->sensorsOff;
	ctrlInputsOff_ = settings->ctrlInputsOff;
	ctrlOutputsOff_ = settings->ctrlOutputsOff;
	

    if(audioVerbose)
        printf("\nLDSP_initAudio()\n");

	//TODO deactivate audio server

	// Initialize connection with playback and capture devices

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
	if(fullDuplex)
	{
		if(initLowLevelAudioStruct(pcmContext.capture)<0)
		{
			// try partial deallocation
			deallocateLowLevelAudioStruct(pcmContext.capture);
			return -3;
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
		fprintf(stderr, "Error:unable to create thread\n");
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
	// Audio struct is totally empty on call, this function allocates memory for it, and then populates it

    // *audioStruct = (audio_struct*) malloc(sizeof(audio_struct));

	// audio_struct* audioStruct = NULL; // Not entirely sure why + if this line is needed
	if (posix_memalign((void**)*&audioStruct, 16, sizeof(audio_struct)) != 0) {
        perror("posix_memalign failed");
    }

    
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
	(*audioStruct)->config.avail_min = 1;
	(*audioStruct)->config.start_threshold = 1; //VIC 60 //settings->periodSize;
	(*audioStruct)->config.stop_threshold = 0;
    (*audioStruct)->config.silence_threshold = 1; //settings->periodSize * settings->periodCount; 
    (*audioStruct)->config.silence_size = 0;


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
    		fromFloatToRaw = fromFloatToRaw_int_NEON; // Set to only be NEON if supported
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
	

	/*
	 * 
	 */
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

	audio_struct->numOfSamples = channels*audio_struct->config.period_size;
	audio_struct->numOfSamples4Multiple = audio_struct->numOfSamples + (4 - audio_struct->numOfSamples % 4) % 4;

	unsigned int formatBits;


	/*
	 * Create a local var to replace frameBytes
	 * Closest multiple of 4 that is >= frameBytes
	 */
    int localFrames = audio_struct->frameBytes + (4 - audio_struct->frameBytes % 4) % 4;


	// allocate buffers
	audio_struct->rawBuffer = malloc(localFrames);
	if(!audio_struct->rawBuffer)
	{
		fprintf(stderr, "Could not allocate rawBuffer\n");
		return -1;
	}

	// AudioBuffer is set here, so this is where we want to ByteAlign

	// numOfSamples = period size * num_channels
	// frame -> number of samples at for each channel
	/*
	 * Want to make sure period size is multiple of 4 -> typically are powers of 2
	 * Unlikely that it isn't a power of 2, but possible
	 * Want to check before we compute numSamples
	*/
	// assert(audio_struct->numOfSamples % 4 == 0); // Assert that numSamples is a multiple of 4

	// Allocate ByteAligned array
	// Question: Should alignment be 16? or 4? Or set in Config?
	// 16 Should be good to stay
	// if (posix_memalign((void**)&audio_struct->audioBuffer, 16, audio_struct->numOfSamples * sizeof(float)) != 0) {
    //     perror("posix_memalign failed");
    //     return -1;
    // }

	/*
	 * Create local var to replace numOfSamples
	 * Closest multiple of 4 that is >= numOfSamples
	 *  *** Q: Is this still needed?
	 */
	audio_struct->audioBuffer = (float*)malloc(sizeof(float)*audio_struct->numOfSamples4Multiple);


	if(!audio_struct->audioBuffer)
	{
		fprintf(stderr, "Could not allocate audioBuffer\n");
		return -2;
	}
	memset(audio_struct->audioBuffer, 0, audio_struct->numOfSamples4Multiple*sizeof(float)); // quiet please!

	// finish audio initialization
	//formatBits = LDSP_pcm_format_to_bits((int)audio_struct->config.format); // replaces/implements pcm_format_to_bits(), that is missing from old tinyalsa
	formatBits = pcm_format_to_bits(audio_struct->config.format); // pcm_format_to_bits doesn't handle 8bit case
	audio_struct->formatBits = formatBits;

	// Max value that we can represent (scaleVal) should be caluclated using formatBits = 24 FOR both S24_LE and S_243LE
	audio_struct->scaleVal = (1 << (formatBits - 1)) - 1;

	/**
	 * physBps is the container 
	 * bps is the bytes of the actual content
	 * 
	 * 	Can use formatBits (from pcm_format_to_bits) to compute physBps
	 * 	physBps = formatBits / 8
	 * 
	 * S24: physBps -> 4, bps -> 3
	 * S24_3LE: physBps -> 3, bps -> 3
	*/

	audio_struct->physBps = formatBits / 8;  // size in bytes of the format var type used to store sample
	// different than this, i.e., number of bytes actually used within that format var type!
	if(audio_struct->config.format != gFormats["S24_3LE"] && audio_struct->config.format != gFormats["S24_3BE"]) // these vars span 32 bits, but only 24 are actually used! -> 3 bytes
		audio_struct->bps = audio_struct->physBps;
	else
		audio_struct->bps = 3;


	/*
	 * Move scaleVal calculation down here, below BPS
	 * Create var physicalFormatBitstemp (local) = bps * 8
	 * Compute scaleVal using that
	 * 	audio_struct->scaleVal = (1 << (physicalFormatBitstemp - 1)) - 1;
	*/

	audio_struct->factorVec = vdupq_n_f32(audio_struct->scaleVal);
	audio_struct->maskVec = vdupq_n_s32(0xff);

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
	// set minimum thread niceness
 	set_niceness(-20, audioVerbose); // only necessary if not real-time, but just in case...

	// set thread priority
	set_priority(LDSPprioOrder_audio, audioVerbose);

	while(!gShouldStop)
	{
		if(fullDuplex)
		{
			if(pcm_read(pcmContext.capture->pcm, pcmContext.capture->rawBuffer, pcmContext.capture->frameBytes)!=0)
			{
				fprintf(stderr, "\nCapture error, aborting...\n");
			}

			fromRawToFloat(pcmContext.capture);
		}

		if(!sensorsOff_)
			readSensors();
		if(!ctrlInputsOff_)
			readCtrlInputs();

		render(userContext, 0);

		fromFloatToRaw(pcmContext.playback);

		if(pcm_write(pcmContext.playback->pcm, pcmContext.playback->rawBuffer, pcmContext.playback->frameBytes)!=0)
		{
			fprintf(stderr, "\nPlayback error, aborting...\n");
		}

		if(!ctrlOutputsOff_)
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

			// printf("%d", res);

			audio_struct->audioBuffer[n] = res/((float)(audio_struct->scaleVal)); // turn int sample into full scale normalized float

			// printf("%f", audio_struct->audioBuffer[n]);
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

		// printf("%d", res);

		fval.i = res; // safe
		audio_struct->audioBuffer[n] = fval.f; // interpret as float

		// printf("%f", audio_struct->audioBuffer[n]);

		sampleBytes += audio_struct->bps; // jump to next sample
	}
}

//TODO optimize: loop unrolling and, when possible, NEON
//VIC on android, it seems that interleaved is the only way to go
// Original - Sh
void fromFloatToRaw_int(audio_struct *audio_struct)
{
	auto start = std::chrono::high_resolution_clock::now();

	unsigned char *sampleBytes = (unsigned char *)audio_struct->rawBuffer; 

	for(unsigned int n=0; n<audio_struct->numOfSamples; n++) 
	{
		int res = audio_struct->scaleVal * audio_struct->audioBuffer[n]; // get actual int sample out of normalized full scale float
		
		byteSplit(sampleBytes, res, audio_struct); // function pointer, splits int into consecutive bytes in either little or big endian

		sampleBytes += audio_struct->bps; // jump to next sample
	}
	// clean up buffer for next period
	memset(audio_struct->audioBuffer, 0, audio_struct->numOfSamples*sizeof(float));

	auto end = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double, std::milli> elapsed = end - start;
	std::cout << "Execution time: " << elapsed.count() << " ms\n";
}


// Loop unrolled:
// void fromFloatToRaw_int(audio_struct *audio_struct)
// {
// 	auto start = std::chrono::high_resolution_clock::now();

// 	unsigned char *sampleBytes = (unsigned char *)audio_struct->rawBuffer; 

// 	for(unsigned int n=0; n<audio_struct->numOfSamples4Multiple; n = n + 4) 
// 	{	
// 		int res[4];
// 		res[0] = audio_struct->scaleVal * audio_struct->audioBuffer[0 + n]; // get actual int sample out of normalized full scale float
// 		res[1] = audio_struct->scaleVal * audio_struct->audioBuffer[1 + n];
// 		res[2] = audio_struct->scaleVal * audio_struct->audioBuffer[2 + n];
// 		res[3] = audio_struct->scaleVal * audio_struct->audioBuffer[3 + n];

// 		byteSplit(sampleBytes, res[0], audio_struct); // function pointer, splits int into consecutive bytes in either little or big endian
// 		sampleBytes += audio_struct->bps; // jump to next sample

// 		byteSplit(sampleBytes, res[1], audio_struct); // function pointer, splits int into consecutive bytes in either little or big endian
// 		sampleBytes += audio_struct->bps; // jump to next sample

// 		byteSplit(sampleBytes, res[2], audio_struct); // function pointer, splits int into consecutive bytes in either little or big endian
// 		sampleBytes += audio_struct->bps; // jump to next sample

// 		byteSplit(sampleBytes, res[3], audio_struct); // function pointer, splits int into consecutive bytes in either little or big endian
// 		sampleBytes += audio_struct->bps; // jump to next sample
// 	}
// 	// clean up buffer for next period
// 	memset(audio_struct->audioBuffer, 0, audio_struct->numOfSamples*sizeof(float));

// 	auto end = std::chrono::high_resolution_clock::now();
// 	std::chrono::duration<double, std::milli> elapsed = end - start;
// 	std::cout << "Execution time: " << elapsed.count() << " ms\n";
// }

// NEON
void fromFloatToRaw_int_NEON(audio_struct *audio_struct)
{
	auto start = std::chrono::high_resolution_clock::now();      
	// YOUR CODE (Full content of body of the function) 
	unsigned char *sampleBytes = (unsigned char *)audio_struct->rawBuffer; 

	for(unsigned int n=0; n<audio_struct->numOfSamples4Multiple; n = n + 4) 
	{	
		// Load the four floating-point inputs into a NEON vector
    	float32x4_t inputVec = vld1q_f32(&(audio_struct->audioBuffer[n]));
		float32x4_t resultVec = vmulq_f32(inputVec, audio_struct->factorVec);

		// int res[4];
		// Truncates float type to int type
		int32x4_t intValues = vcvtq_s32_f32(resultVec);

		// Moves intValues to res
		// vst1q_s32(res, intValues);

		// Will be changed to be ByteSplit()
		// byteSplit_littleEndian_unrolled(&sampleBytes, res, audio_struct);
		byteSplit_littleEndian_NEON(&sampleBytes, intValues, audio_struct);
		// byteSplit_bigEndian_NEON(&sampleBytes, intValues, audio_struct);

		// byteSplit(sampleBytes, res[0], audio_struct);
		// // sampleBytes += audio_struct->physBps; // jump to next sample ** COnfirm I need to jump here?

		// byteSplit(sampleBytes, res[1], audio_struct);
		// // sampleBytes += audio_struct->physBps; // jump to next sample

		// byteSplit(sampleBytes, res[2], audio_struct);
		// // sampleBytes += audio_struct->physBps; // jump to next sample

		// byteSplit(sampleBytes, res[3], audio_struct);
		// sampleBytes += audio_struct->physBps; // jump to next sample
	}
	// clean up buffer for next period
	memset(audio_struct->audioBuffer, 0, audio_struct->numOfSamples*sizeof(float));

	auto end = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double, std::milli> elapsed = end - start;
	std::cout << "Execution time: " << elapsed.count() << " ms\n";
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
		// printf("%f", audio_struct->audioBuffer[n]);
		fval.f = audio_struct->audioBuffer[n]; // safe, cos float is at least 32 bits
		int res = fval.i; // interpret as int
		
		// printf("%d", res);

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
	for (int i = 0; i < audio_struct->bps; i++) {
		*(sampleBytes + i) = (value >>  i * 8) & 0xff; // ff is a byte	
	}
}

inline void byteSplit_littleEndian_unrolled(unsigned char** sampleBytes, int res[4], struct audio_struct* audio_struct)
{
	int bps = audio_struct->bps;
	for(int i = 0; i < audio_struct->bps; i++) {
        *(*sampleBytes + i) = (res[0] >>  i * 8) & 0xff;
        
        *(*sampleBytes + 1 * bps + i) = (res[1] >>  i * 8) & 0xff;

        *(*sampleBytes + 2 * bps + i) = (res[2] >>  i * 8) & 0xff;

        *(*sampleBytes + 3 * bps + i) = (res[3] >>  i * 8) & 0xff;
    }
	*sampleBytes += (4 *audio_struct->physBps);
}

// NEON Version
inline void byteSplit_littleEndian_NEON(unsigned char** sampleBytes, int32x4_t values, struct audio_struct* audio_struct)
{

	int bps = audio_struct->bps;
	for (int i = 0; i < bps; i++) {
		int32x4_t shiftAmountVec = vdupq_n_s32(-i*8); // Cannot be created elsewhere because it uses i
		int32x4_t shifted_values = vshlq_s32(values, shiftAmountVec);

		int res[4];

		int32x4_t maskedValues = vandq_s32(shifted_values, audio_struct->maskVec);

	 	vst1q_s32(res, maskedValues);

		*(*sampleBytes + i) = res[0];

		*(*sampleBytes + 1 * bps + i) = res[1];

		*(*sampleBytes + 2 * bps + i) = res[2];

		*(*sampleBytes + 3 * bps + i) = res[3];
	}
	*sampleBytes += (4 *audio_struct->physBps);

}

// little endian -> byte_n-1, byte_n-2, ..., byte_0
inline void byteSplit_bigEndian(unsigned char *sampleBytes, int value, struct audio_struct* audio_struct) 
/**
 * ABC -> A,B,C
 * 	Try bigEndian -> but confusion we had about physBps
 * 	Run with -v, list formats that are supported - Might also support BE
 * 
*/
{
	for (int i = 0; i <audio_struct->bps; i++)
		*(sampleBytes + audio_struct->physBps - 1 - i) = (value >> i * 8) & 0xff; 
}

inline void byteSplit_bigEndian_NEON(unsigned char **sampleBytes, int32x4_t values, struct audio_struct* audio_struct) 
/**
 *  ** Yet to be tested **
*/
{
	int bps = audio_struct->bps;
	for (int i = 0; i < bps; i++) {
		
		// NEON Right shift requires int to be a compile-time constant, so need to left shift by negative
		int32x4_t shiftAmountVec = vdupq_n_s32(-i*8);
		int32x4_t shifted_values = vshlq_s32(values, shiftAmountVec);

		int res[4];

		int32x4_t maskedValues = vandq_s32(shifted_values, audio_struct->maskVec);

	 	vst1q_s32(res, maskedValues);

		// Can this part be vectorized?
		int physBps = audio_struct->physBps;
		*(*sampleBytes + physBps - 1 - i) = res[0];

		*(*sampleBytes + physBps - 1 + 1 * bps - i) = res[1];

		*(*sampleBytes + physBps - 1 + 2 * bps - i) = res[2];

		*(*sampleBytes + physBps - 1 + 3 * bps - i) = res[3];
	}
	*sampleBytes += (4 * audio_struct->physBps);
}
