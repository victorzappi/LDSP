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

// This is just an example on how to carry out a raw paralle FFT and iFFT on the Neon via the Ne10 library
// Notice that in LDSP there is a way more convenient Ne10 parallel FFT implementation under libraries/Fft
// Example usage can be found in examples/Audio/fft_boilerplate

#include "LDSP.h"
#include <math.h> // sin
#include "NE10.h"

#define SAMPLES 16

float frequency = 440.0;
float amplitude = 0.2;

//------------------------------------------------
float phase;
float inverseSampleRate;


bool setup(LDSPcontext *context, void *userData)
{
    // Initialize Ne10 library
    if(ne10_init() != NE10_OK)
    {
        printf("Failed to initialize Ne10\n");
        return false;
    }

    // Declare variables for FFT
    ne10_fft_cfg_float32_t cfg;
    ne10_fft_cpx_float32_t src[SAMPLES]; // A source array of input data
    ne10_fft_cpx_float32_t dst[SAMPLES]; // A destination array for the transformed data
    ne10_fft_cpx_float32_t idst[SAMPLES]; // An inverse destination array for the transformed data
    int fft_size = 1024; // Size of the FFT
        
    // Allocate the FFT configuration
    // cfg = ne10_fft_alloc_c2c_float32_c(fft_size);
    cfg = ne10_fft_alloc_c2c_float32_c(SAMPLES);
    if(cfg == NULL)
    {
        printf("Failed to allocate Ne10 FFT configuration\n");
        return false;
    }

    printf("Successfully allocated Ne10 FFT configuration for size %d\n", fft_size);

    // generated random complex input
    for(int i=0; i<SAMPLES; i++)
    {
        src[i].r = (ne10_float32_t)rand() / (ne10_float32_t)RAND_MAX * 50.0f;
        src[i].i = (ne10_float32_t)rand() / (ne10_float32_t)RAND_MAX * 50.0f;
    }

    printf("Randomly generated complex input (time domain):\n");
    for(int i=0; i<SAMPLES; i++)
        printf( "IN[%2d]: %10.4f + %10.4fi\n", i, src[i].r, src[i].i);

    printf("Performing Ne10 FFT...\n");
    // Perform the FFT (for an IFFT, the last parameter should be `1`)
    ne10_fft_c2c_1d_float32(dst, src, cfg, 0);

    printf("Ne10 FFT complex output (frequency domain):\n");
    for(int i=0; i<SAMPLES; i++)
        printf("OUT[%2d]: %10.4f + %10.4fi\n", i, dst[i].r, dst[i].i);

    printf("Performing Ne10 iFFT...\n");
    // Perform the iFFT (for an IFFT, the last parameter should be `1`)
    ne10_fft_c2c_1d_float32(idst, dst, cfg, 1);

    printf("Ne10 iFFT complex output (back to time domain):\n");
    for(int i=0; i<SAMPLES; i++)
        printf( "IN[%2d]: %10.4f + %10.4fi\n", i, idst[i].r, idst[i].i);

    // Free the FFT configuration resources
    ne10_fft_destroy_c2c_float32(cfg);

    inverseSampleRate = 1.0 / context->audioSampleRate;
    phase = 0.0;

    printf("Now generating a CPU real-time sine tone...\n");
    
    return true;
}

void render(LDSPcontext *context, void *userData)
{
    for(int n=0; n<context->audioFrames; n++)
    {
        float out = amplitude * sinf(phase);
        phase += 2.0f * (float)M_PI * frequency * inverseSampleRate;
        while(phase > 2.0f *M_PI)
        phase -= 2.0f * (float)M_PI;
        for(int chn=0; chn<context->audioOutChannels; chn++)
            audioWrite(context, n, chn, out);
    }
}

void cleanup(LDSPcontext *context, void *userData)
{

}