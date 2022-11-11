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

// a stripped down version of alsa asound.h, to reimplement some of the functionalities of recent tinyalsa lib varsions that are not available in 
// the tinyalsa implementation used in Android 4.0, 4.0.1, 4.0.2 [ICE_CREAM_SANDWICH], API level 14

#ifndef TINY_ASOUND_H_
#define TINY_ASOUND_H_

#include <sys/ioctl.h> // ioctl()
#include <fcntl.h> // for open()
#include <unistd.h> // for close()
// from asound.h File Reference, https://docs.huihoo.com/doxygen/linux/kernel/3.7/asound_8h.html



// for LDSP_pcm_link/unlink()
#define SNDRV_PCM_IOCTL_LINK   _IOW('A', 0x60, int)
#define SNDRV_PCM_IOCTL_UNLINK   _IO('A', 0x61)


//------------------------------------------------------------------------------------
// for LDSP_pcm_params_get() and LDSP_pcm_params_get_min/max()
#define SNDRV_PCM_IOCTL_HW_REFINE	_IOWR('A', 0x10, struct snd_pcm_hw_params)

#define	SNDRV_PCM_HW_PARAM_ACCESS	0	/* Access type */
#define	SNDRV_PCM_HW_PARAM_FORMAT	1	/* Format */
#define	SNDRV_PCM_HW_PARAM_SUBFORMAT	2	/* Subformat */
#define	SNDRV_PCM_HW_PARAM_FIRST_MASK	SNDRV_PCM_HW_PARAM_ACCESS
#define	SNDRV_PCM_HW_PARAM_LAST_MASK	SNDRV_PCM_HW_PARAM_SUBFORMAT

#define	SNDRV_PCM_HW_PARAM_SAMPLE_BITS	8	/* Bits per sample */
#define	SNDRV_PCM_HW_PARAM_FRAME_BITS	9	/* Bits per frame */
#define	SNDRV_PCM_HW_PARAM_CHANNELS	10	/* Channels */
#define	SNDRV_PCM_HW_PARAM_RATE		11	/* Approx rate */
#define	SNDRV_PCM_HW_PARAM_PERIOD_TIME	12	/* Approx distance between interrupts in us*/
#define	SNDRV_PCM_HW_PARAM_PERIOD_SIZE	13	/* Approx frames between interrupts */
#define	SNDRV_PCM_HW_PARAM_PERIOD_BYTES	14	/* Approx bytes between * interrupts */
#define	SNDRV_PCM_HW_PARAM_PERIODS	15	/* Approx interrupts per * buffer */
#define	SNDRV_PCM_HW_PARAM_BUFFER_TIME	16	/* Approx duration of buffer in us */
#define	SNDRV_PCM_HW_PARAM_BUFFER_SIZE	17	/* Size of buffer in frames */
#define	SNDRV_PCM_HW_PARAM_BUFFER_BYTES	18	/* Size of buffer in bytes */
#define	SNDRV_PCM_HW_PARAM_TICK_TIME	19	/* Approx tick duration in us */
#define	SNDRV_PCM_HW_PARAM_FIRST_INTERVAL	SNDRV_PCM_HW_PARAM_SAMPLE_BITS
#define	SNDRV_PCM_HW_PARAM_LAST_INTERVAL	SNDRV_PCM_HW_PARAM_TICK_TIME

#define SNDRV_MASK_MAX	256

struct snd_mask {
	__u32 bits[(SNDRV_MASK_MAX+31)/32];
};

struct snd_interval {
	unsigned int min, max;
	unsigned int openmin:1,
		     openmax:1,
		     integer:1,
		     empty:1;
};

typedef unsigned long snd_pcm_uframes_t;

struct snd_pcm_hw_params {
	unsigned int flags;
	struct snd_mask masks[SNDRV_PCM_HW_PARAM_LAST_MASK -
			       SNDRV_PCM_HW_PARAM_FIRST_MASK + 1];
	struct snd_mask mres[5];	/* reserved masks */
	struct snd_interval intervals[SNDRV_PCM_HW_PARAM_LAST_INTERVAL -
				        SNDRV_PCM_HW_PARAM_FIRST_INTERVAL + 1];
	struct snd_interval ires[9];	/* reserved intervals */
	unsigned int rmask;		/* W: requested masks */
	unsigned int cmask;		/* R: changed masks */
	unsigned int info;		/* R: Info flags for returned setup */
	unsigned int msbits;		/* R: used most significant bits */
	unsigned int rate_num;		/* R: rate numerator */
	unsigned int rate_den;		/* R: rate denominator */
	snd_pcm_uframes_t fifo_size;	/* R: chip FIFO size in frames */
	unsigned char reserved[64];	/* reserved for future */
};

#endif /* TINY_ASOUND_H_ */