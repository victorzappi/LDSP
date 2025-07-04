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

// Note that this project runs a parallel implementation of the FFT (via the Ne10 library), unless LDSP is configured with the optional flag --no-neon-fft
// in which case the FFT is carried out via the fftw library

#include <LDSP.h>
#include <libraries/Fft/Fft.h>
#include <iostream>


Fft fft;

std::vector<float> fft_in;
std::vector<float> real;
std::vector<float> imag;

int n_fft;

bool setup(LDSPcontext *context, void *userData)
{
    
    n_fft = context->audioFrames;

    if (!fft.setup(n_fft)) {
        std::cout << "Error initializing FFT object" << std::endl;
        return false;
    }

    fft_in.resize(n_fft);
    real.resize(n_fft);
    imag.resize(n_fft);
    
    return true;
}


void render(LDSPcontext *context, void *userData)
{
    // Sum channels to a mono signal
    float sum;
    for (int i = 0; i < n_fft; i++) 
    {
        sum = 0.0;
        for (int channel = 0; channel < context->audioInChannels; channel++)
            sum += audioRead(context, i, channel);
        fft_in[i] = sum / (float) context->audioInChannels;
    }

    // Perform the FFT
    fft.fft(fft_in);


    // ... Do something with the Spectral Data...
    for (int i = 0; i < n_fft; i++) 
    {
        real[i] = fft.fdr(i);
        imag[i] = fft.fdi(i);
    }

    for (int i = 0; i < n_fft; i++) 
    {
        for (int channel = 0; channel < context->audioOutChannels; channel++)
            audioWrite(context, i, channel, 0);
    }  
}

void cleanup(LDSPcontext *context, void *userData)
{
}