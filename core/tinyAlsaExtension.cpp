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

// these functions are inpsired to latest version of tinyalsa and replace the ones missing from the version that we are using


//TODO remove these functions and file once we switch to tinyalsa source

#include "tinyalsaAudio.h"

// from asoundlib.h, implementation of tinyalsa from LineageOS 14
// equivalent to Android 7.0 [NOUGAT], API level 24
int LDSP_pcm_link(audio_struct *audio_struct_p, audio_struct *audio_struct_c)
{
	if(!ioctl(audio_struct_c->fd, SNDRV_PCM_IOCTL_LINK, audio_struct_p->fd))
	{
		fprintf(stderr, "Failed to link capture pcm %u,%u, with playback pcm %u,%u\n", audio_struct_c->card, audio_struct_c->device, audio_struct_p->card, audio_struct_p->device);
		return -2;
	}
    return 0;
}

// from asoundlib.h, implementation of tinyalsa from LineageOS 14
// equivalent to Android 7.0 [NOUGAT], API level 24
void LDSP_pcm_unlink(audio_struct *audio_struct)
{
    ioctl(audio_struct->fd, SNDRV_PCM_IOCTL_UNLINK);
}

//--------------------------------------------------------------------------------

unsigned int LDSP_pcm_format_to_bits(int format)
{
    switch (format) 
    {
        case LDSP_pcm_format::PCM_FORMAT_S32_LE:
        case LDSP_pcm_format::PCM_FORMAT_S32_BE:
        case LDSP_pcm_format::PCM_FORMAT_S24_LE:
        case LDSP_pcm_format::PCM_FORMAT_S24_BE:
        case LDSP_pcm_format::PCM_FORMAT_FLOAT_LE:
        case LDSP_pcm_format::PCM_FORMAT_FLOAT_BE:
            return 32;
        case LDSP_pcm_format::PCM_FORMAT_S24_3LE:
        case LDSP_pcm_format::PCM_FORMAT_S24_3BE:
            return 24;
        default:
        case LDSP_pcm_format::PCM_FORMAT_S16_LE:
        case LDSP_pcm_format::PCM_FORMAT_S16_BE:
            return 16;
        case LDSP_pcm_format::PCM_FORMAT_S8:
            return 8;
    };

    return -1;
}

//--------------------------------------------------------------------------------

static void LDSP_param_init(struct snd_pcm_hw_params *p); 

// from asoundlib.h, implementation of tinyalsa from LineageOS 14
// equivalent to Android 7.0 [NOUGAT], API level 24
struct LDSP_pcm_params *LDSP_pcm_params_get(unsigned int card, unsigned int device, unsigned int flags)
{
    struct snd_pcm_hw_params *params;
    char fn[256];
    int fd;

    snprintf(fn, sizeof(fn), "/dev/snd/pcmC%uD%u%c", card, device,
             flags & PCM_IN ? 'c' : 'p');

    fd = open(fn, O_RDWR);
    if (fd < 0) {
        fprintf(stderr, "cannot open device '%s'\n", fn);
        goto err_open;
    }

    params = (snd_pcm_hw_params *)calloc(1, sizeof(struct snd_pcm_hw_params));
    if (!params)
        goto err_calloc;

    LDSP_param_init(params);
    if (ioctl(fd, SNDRV_PCM_IOCTL_HW_REFINE, (void *)params)) {
        fprintf(stderr, "SNDRV_PCM_IOCTL_HW_REFINE error (%d)\n", errno);
        goto err_hw_refine;
    }

    close(fd);

    return (struct LDSP_pcm_params *)params;

err_hw_refine:
    free(params);
err_calloc:
    close(fd);
err_open:
    return NULL;
}

// from asoundlib.h, implementation of tinyalsa from LineageOS 14
// equivalent to Android 7.0 [NOUGAT], API level 24
static inline struct snd_mask *LDSP_param_to_mask(struct snd_pcm_hw_params *p, int n)
{
    return &(p->masks[n - SNDRV_PCM_HW_PARAM_FIRST_MASK]);
}

// from asoundlib.h, implementation of tinyalsa from LineageOS 14
// equivalent to Android 7.0 [NOUGAT], API level 24
static inline struct snd_interval *LDSP_param_to_interval(struct snd_pcm_hw_params *p, int n)
{
    return &(p->intervals[n - SNDRV_PCM_HW_PARAM_FIRST_INTERVAL]);
}

// from asoundlib.h, implementation of tinyalsa from LineageOS 14
// equivalent to Android 7.0 [NOUGAT], API level 24
static void LDSP_param_init(struct snd_pcm_hw_params *p)
{
    int n;

    memset(p, 0, sizeof(*p));
    for (n = SNDRV_PCM_HW_PARAM_FIRST_MASK;
         n <= SNDRV_PCM_HW_PARAM_LAST_MASK; n++) {
            struct snd_mask *m = LDSP_param_to_mask(p, n);
            m->bits[0] = ~0;
            m->bits[1] = ~0;
    }
    for (n = SNDRV_PCM_HW_PARAM_FIRST_INTERVAL;
         n <= SNDRV_PCM_HW_PARAM_LAST_INTERVAL; n++) {
            struct snd_interval *i = LDSP_param_to_interval(p, n);
            i->min = 0;
            i->max = ~0;
    }
    p->rmask = ~0U;
    p->cmask = 0;
    p->info = ~0U;
}

//--------------------------------------------------------------------------------

static unsigned int LDSP_param_get_min(struct snd_pcm_hw_params *p, int n);

// adapted from asoundlib.h, implementation of tinyalsa from LineageOS 14
// equivalent to Android 7.0 [NOUGAT], API level 24
unsigned int LDSP_pcm_params_get_min(struct LDSP_pcm_params *pcm_params, LDSP_pcm_param param)
{
    struct snd_pcm_hw_params *params = (struct snd_pcm_hw_params *)pcm_params;

    if (!params)
        return 0;

    if (param < 0)
        return 0;

    return LDSP_param_get_min(params, param);
}

// from asoundlib.h, implementation of tinyalsa from LineageOS 14
// equivalent to Android 7.0 [NOUGAT], API level 24
static inline int LDSP_param_is_interval(int p)
{
    return (p >= SNDRV_PCM_HW_PARAM_FIRST_INTERVAL) &&
        (p <= SNDRV_PCM_HW_PARAM_LAST_INTERVAL);
}

// from asoundlib.h, implementation of tinyalsa from LineageOS 14
// equivalent to Android 7.0 [NOUGAT], API level 24
static inline struct snd_interval *param_to_interval(struct snd_pcm_hw_params *p, int n)
{
    return &(p->intervals[n - SNDRV_PCM_HW_PARAM_FIRST_INTERVAL]);
}

// from asoundlib.h, implementation of tinyalsa from LineageOS 14
// equivalent to Android 7.0 [NOUGAT], API level 24
static unsigned int LDSP_param_get_min(struct snd_pcm_hw_params *p, int n)
{
    if (LDSP_param_is_interval(n)) {
        struct snd_interval *i = param_to_interval(p, n);
        return i->min;
    }
    return 0;
}

//--------------------------------------------------------------------------------

static unsigned int LDSP_param_get_max(struct snd_pcm_hw_params *p, int n);

// adapted from asoundlib.h, implementation of tinyalsa from LineageOS 14
// equivalent to Android 7.0 [NOUGAT], API level 24
unsigned int LDSP_pcm_params_get_max(struct LDSP_pcm_params *pcm_params, LDSP_pcm_param param)
{
    struct snd_pcm_hw_params *params = (struct snd_pcm_hw_params *)pcm_params;

    if (!params)
        return 0;

    if (param < 0)
        return 0;

    return LDSP_param_get_max(params, param);
}

// from asoundlib.h, implementation of tinyalsa from LineageOS 14
// equivalent to Android 7.0 [NOUGAT], API level 24
static unsigned int LDSP_param_get_max(struct snd_pcm_hw_params *p, int n)
{
    if (LDSP_param_is_interval(n)) {
        struct snd_interval *i = LDSP_param_to_interval(p, n);
        return i->max;
    }
    return 0;
}

//--------------------------------------------------------------------------------

// from asoundlib.h, implementation of tinyalsa from LineageOS 14
// equivalent to Android 7.0 [NOUGAT], API level 24
void LDSP_pcm_params_free(struct LDSP_pcm_params *pcm_params)
{
    struct snd_pcm_hw_params *params = (struct snd_pcm_hw_params *)pcm_params;

    if (params)
        free(params);
}

//TODO re-implement check on formats using LineageOS pcm.c as ref

