#include "LDSP.h"
#include <math.h> // sin
#include "NE10.h" // Include the Ne10 header file

float frequency = 440.0;
float amplitude = 0.2;

//------------------------------------------------
float phase;
float inverseSampleRate;

#define SAMPLES 16



bool setup(LDSPcontext *context, void *userData)
{
    // Initialize Ne10 library
    if (ne10_init() != NE10_OK)
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
    if (cfg == NULL)
    {
        printf("Failed to allocate FFT configuration\n");
        return false;
    }

    // printf("Successfully allocated FFT configuration for size %d\n", fft_size);

    for (int i = 0; i < SAMPLES; i++)
    {
        src[i].r = (ne10_float32_t)rand() / (ne10_float32_t)RAND_MAX * 50.0f;
        src[i].i = (ne10_float32_t)rand() / (ne10_float32_t)RAND_MAX * 50.0f;
    }

    // Display the results
    for (int i = 0; i < SAMPLES; i++)
    {
        printf( "IN[%2d]: %10.4f + %10.4fi\n", i, src[i].r, src[i].i);
        //printf("OUT[%2d]: %10.4f + %10.4fi\n", i, dst[i].r, dst[i].i);
    }

    // Perform the FFT (for an IFFT, the last parameter should be `1`)
    ne10_fft_c2c_1d_float32(dst, src, cfg, 0);

    // Display the results
    for (int i = 0; i < SAMPLES; i++)
    {
        //printf( "IN[%2d]: %10.4f + %10.4fi\t", i, src[i].r, src[i].i);
        printf("OUT[%2d]: %10.4f + %10.4fi\n", i, dst[i].r, dst[i].i);
    }

    // Perform the iFFT (for an IFFT, the last parameter should be `1`)
    ne10_fft_c2c_1d_float32(idst, dst, cfg, 1);

    // Display the results
    for (int i = 0; i < SAMPLES; i++)
    {
        printf( "IN[%2d]: %10.4f + %10.4fi\n", i, idst[i].r, idst[i].i);
        //printf("OUT[%2d]: %10.4f + %10.4fi\n", i, dst[i].r, dst[i].i);
    }

    // Free the FFT configuration resources
    ne10_fft_destroy_c2c_float32(cfg);

    inverseSampleRate = 1.0 / context->audioSampleRate;
    phase = 0.0;
    
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