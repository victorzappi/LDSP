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

// Polyphonic sine‑wave example for LDSP (4 voices)
// -------------------------------------------------
// Phase increment and wrapping done with ARM NEON intrinsics
// Regular libm sinf() for waveform (easy to read; good accuracy)

#include "LDSP.h"
#include <arm_neon.h>
#include <cmath>   // sinf()

// ------------------------------------------------------------------------
// Configuration
// ------------------------------------------------------------------------
constexpr int NUM_VOICES = 4;                   
static float kFrequencies[NUM_VOICES] = {261.63f, 311.13f, 392.00f, 415.30f}; // C‑D#‑G‑G#
static float kAmplitudes [NUM_VOICES] = {0.10f , 0.10f , 0.10f , 0.10f };

alignas(16) float phases[NUM_VOICES]; // stack buffer for SIMD → scalar

// ------------------------------------------------------------------------
// Global DSP state
// ------------------------------------------------------------------------
static float32x4_t phase_v;       // Phase for 4 voices
static float32x4_t delta_v;       // Phase increment per sample (2πf/Fs)
static float32x4_t two_pi_v;      // 2π constant


bool setup(LDSPcontext* context, void* userData)
{
    float invSR = 1.0f / context->audioSampleRate;

    two_pi_v = vdupq_n_f32(2.0f * (float)M_PI);
    phase_v  = vdupq_n_f32(0.0f);

    float32x4_t freq_v = vld1q_f32(kFrequencies);
    float32x4_t scale  = vdupq_n_f32(2.0f * (float)M_PI * invSR);
    delta_v            = vmulq_f32(freq_v, scale);

    return true;
}

void render(LDSPcontext* context, void* userData)
{
    for (int n = 0; n < context->audioFrames; ++n)
    {
        // 1. Copy SIMD phases to scalar array (single 128‑bit store)
        vst1q_f32(phases, phase_v);

        // 2. Sum voices (scalar sinf)
        float out = 0.0f;
        for(int i=0; i<NUM_VOICES; i++)
            out += kAmplitudes[i] * sinf(phases[i]);

        // 3. Output mixed sample to all channels
        for (int ch=0; ch<context->audioOutChannels; ch++)
            audioWrite(context, n, ch, out);

        // 4. SIMD phase increment
        phase_v = vaddq_f32(phase_v, delta_v);

        // 5. SIMD wrap back to 0..2π
        uint32x4_t mask = vcgtq_f32(phase_v, two_pi_v); 
        phase_v = vsubq_f32(phase_v, vbslq_f32(mask, two_pi_v, vdupq_n_f32(0.0f)));
    }
}

void cleanup(LDSPcontext* context, void* userData)
{
    // Nothing to do
}