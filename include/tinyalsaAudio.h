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

#ifndef TINY_ALSA_AUDIO_H_
#define TINY_ALSA_AUDIO_H_

#include"LDSP.h"

#include <tinyalsa/asoundlib.h>
#include "enums.h"

// wraps tinyalsa pmc_config
struct LDSP_pcm_config {
    unsigned int channels;
    unsigned int rate;
    unsigned int period_size;
    unsigned int period_count;
    enum pcm_format format;

    /* Values to use for the ALSA start, stop and silence thresholds, and
     * silence size.  Setting any one of these values to 0 will cause the
     * default tinyalsa values to be used instead.
     * Tinyalsa defaults are as follows.
     *
     * start_threshold   : period_count * period_size
     * stop_threshold    : period_count * period_size
     * silence_threshold : 0
     * silence_size      : 0
     */
    unsigned int start_threshold;
    unsigned int stop_threshold;
    unsigned int silence_threshold;
    unsigned int silence_size;
    /* Minimum number of frames available before pcm_mmap_write() will actually
     * write into the kernel buffer. Only used if the stream is opened in mmap mode
     * (pcm_open() called with PCM_MMAP flag set).   Use 0 for default.
     */
    int avail_min;
};

struct audio_struct {
    unsigned int card;
    unsigned int device;
    int flags;
    /* LDSP_ */pcm_config config;
    unsigned int frameBytes;
    unsigned int numOfSamples;
    pcm *pcm;
	int fd;
	void *rawBuffer;
	float *audioBuffer;
	unsigned int formatBits;
	unsigned int scaleVal;
	unsigned int bps;
	unsigned int physBps;
    int mask; // used for capture only
};

struct LDSPpcmContext {
    audio_struct *playback;
    audio_struct *capture;
};

struct LDSPinternalContext {
	float *audioIn;
	float *audioOut;
	uint32_t audioFrames;
	uint32_t audioInChannels;
	uint32_t audioOutChannels;
	float audioSampleRate;
    float *sensors; 
    int *ctrlInputs;
    float *ctrlOutputs;
    uint32_t sensorChannels;
    uint32_t ctrlInChannels;
    uint32_t ctrlOutChannels;
    bool *sensorsSupported;
    bool *buttonsSupported;
    bool *ctrlOutputsSupported;
    bool screenGetStateSupported;
    string *sensorsDetails;
    float controlSampleRate; // sensors and output devices
    multiTouchInfo *mtInfo;
	//uint64_t audioFramesElapsed;
    //operator LDSPcontext () {return *(LDSPcontext*)this;}
    string projectName;
};



// wrapper of tinyalsa pmc_format, to ensure compatibility with older and newer versions of tinyalsa
// this is a special wrapper, that permits to print the ENUM ad to cycle via indices
//VIC note that until we update the source of tinnyalsa to latest version, BE entries will not work
ENUM(LDSP_pcm_format, short, 
    //INVALID = -1,
    S16_LE = 0, /** Signed 16-bit, little endian */
    S32_LE,     /** Signed, 32-bit, little endian */ // last supported format from Android 4.0 ICE_CREAM_SANDWICH [API level 14] to Android 4.2.2 JELLY_BEAN_MR1 [API level 17] 
    S8,         /** Signed, 8-bit */
    S24_LE,     /** Signed, 24-bit (32-bit in memory), little endian */ // last supported format from Android 4.3 JELLY_BEAN_MR2 [API level 18] to Android 4.4 KITKAT [API level 19] 
    S24_3LE,    /** Signed, 24-bit, little endian */ // last supported format from Android 5.0 LOLLIPOP [API level 21] to Android 13 TIRAMISU [API level 33] 
    S16_BE,     /** Signed, 16-bit, big endian */ // from here on, formats supported by tinyalsa nut not by current Android implmentaitons of libtinyalsa
    S24_BE,     /** Signed, 24-bit (32-bit in memory), big endian */
    S24_3BE,    /** Signed, 24-bit, big endian */
    S32_BE,     /** Signed, 32-bit, big endian */
    FLOAT_LE,   /** 32-bit float, little endian */
    FLOAT_BE,   /** 32-bit float, big endian */
    MAX         /** Max of the enumeration list, not an actual format. */ //VIC for now
)


// re-implements tinyalsa pcm_link()
int LDSP_pcm_link(audio_struct *audio_struct_p, audio_struct *audio_struct_c);
// re-implements tinyalsa pcm_unlink()
void LDSP_pcm_unlink(audio_struct *audio_struct);

#endif /* TINY_ALSA_AUDIO_H_ */