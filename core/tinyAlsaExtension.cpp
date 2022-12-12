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

//VIC these functions are inpsired to latest version of tinyalsa and replace the ones missing from the version that we are using


//TODO remove these functions once we update our copy of tinyalsa source to latest version

#include "tinyalsaAudio.h"

#include <sys/ioctl.h> // for pcm_link/unlink()
// from asound.h File Reference, https://docs.huihoo.com/doxygen/linux/kernel/3.7/asound_8h.html
#define SNDRV_PCM_IOCTL_LINK   _IOW('A', 0x60, int)
#define SNDRV_PCM_IOCTL_UNLINK   _IO('A', 0x61)

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
