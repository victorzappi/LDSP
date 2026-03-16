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


/*
 * NAM WaveNet example for LDSP
 * Processes a guitar DI recording through a Neural Amp Modeler WaveNet model.
 *
 * Uses NeuralAmpModelerCore to load a .nam file and run block-based inference.
 * The model is purely convolutional (no recurrence), so it processes
 * a full audio block at a time.
 *
 * Model: Kustom K150-1 (Ch1 High Input OD Boost Bright Pot Pulled) --- Feather model
 * The .nam file can be downloaded from here:
 * https://www.tone3000.com/tones/kustom-k150-1-amp-head-only-53113
 * and should be placed in the project's folder.
 * Any NAM model can be used, just be mindful of the computational load.
 *
 * This code example uses this sound from freesound:
 * Clean Electric Guitar by guitarman213
 * https://freesound.org/s/715794/ -- License: Creative Commons 0
 * The original file has been normalized, exported as a mono track
 * and resampled at 48 kHz.
 */

#include "LDSP.h"
#include "MonoFilePlayer.h"
#include "NAM/get_dsp.h"
#include <filesystem>

// Audio file to process
string filename = "715794__guitarman213__clean-electric-guitar_mono_norm_48k.wav";

// NAM model file
string modelFilename = "Kustom K150-1 (Ch1 High Input OD Boost Bright Pot Pulled).nam"; // download here: https://www.tone3000.com/tones/kustom-k150-1-amp-head-only-53113 
// or use any other NAM model from https://www.tone3000.com/

float amplitude = 0.3;

//------------------------------------

MonoFilePlayer player;
std::unique_ptr<nam::DSP> model;

// Buffers for block-based NAM processing
double *inputBuffer = nullptr;
double *outputBuffer = nullptr;
double *inputPtr = nullptr;
double *outputPtr = nullptr;

bool setup(LDSPcontext *context, void *userData)
{
    // Load the audio file
    if(!player.setup(filename, true, true))
    {
        printf("Error loading audio file '%s'\n", filename.c_str());
        return false;
    }

    // Load the NAM model from .nam file
    model = nam::get_dsp(std::filesystem::path(modelFilename));
    if(model == nullptr)
    {
        printf("Error loading NAM model '%s'\n", modelFilename.c_str());
        return false;
    }

    // Initialize the model with sample rate and block size
    model->Reset(context->audioSampleRate, context->audioFrames);
    model->prewarm();

    printf("NAM model loaded: %s\n", modelFilename.c_str());

    // Allocate processing buffers (one block each)
    inputBuffer = new double[context->audioFrames];
    outputBuffer = new double[context->audioFrames];
    inputPtr = inputBuffer;
    outputPtr = outputBuffer;

    return true;
}

void render(LDSPcontext *context, void *userData)
{
    // Fill input buffer from file player
    for(int n = 0; n < context->audioFrames; n++)
        inputBuffer[n] = (double)player.process();

    // Process the entire block through NAM (expects double**)
    model->process(&inputPtr, &outputPtr, context->audioFrames);

    // Write output to both channels
    for(int n = 0; n < context->audioFrames; n++)
    {
        float out = (float)(outputBuffer[n] * amplitude);

        audioWrite(context, n, 0, out);
        audioWrite(context, n, 1, out);
    }
}

void cleanup(LDSPcontext *context, void *userData)
{
    delete[] inputBuffer;
    delete[] outputBuffer;
    model.reset();
}