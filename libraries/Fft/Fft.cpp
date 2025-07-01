#include "Fft.h"
#include <vector>
#include <cstring>

#ifdef NE10_FFT
#include <libraries/ne10/NE10.h>
//--------------------------------------------------------------------------------
// NE10-based implementation
//--------------------------------------------------------------------------------

/**
 * Initialize NE10 FFT: allocate buffers and create configuration.
 * @param len Transform length (must be power of two).
 * @return 0 on success, -1 on failure.
 */
int Fft::setup(unsigned int len)
{
    if (!isPowerOfTwo(len))
        return -1;
    cleanup();
    length = len;
    timeDomain      = (ne10_float32_t*)         NE10_MALLOC(length * sizeof(ne10_float32_t));
    frequencyDomain = (ne10_fft_cpx_float32_t*) NE10_MALLOC(length * sizeof(ne10_fft_cpx_float32_t));
    cfg             = ne10_fft_alloc_r2c_float32(length);
    if (!timeDomain || !frequencyDomain || !cfg) {
        cleanup();
        return -1;
    }
    return 0;
}

/**
 * Cleanup NE10 FFT: free buffers and destroy configuration.
 */
void Fft::cleanup()
{
    NE10_FREE(timeDomain);
    timeDomain      = nullptr;
    NE10_FREE(frequencyDomain);
    frequencyDomain = nullptr;
    ne10_fft_destroy_r2c_float32(cfg);
    cfg              = nullptr;
    length           = 0;
}

/**
 * Perform NE10 forward FFT on internally stored time-domain data.
 */
void Fft::fft()
{
    ne10_fft_r2c_1d_float32_neon(frequencyDomain, timeDomain, cfg);
}

/**
 * Perform NE10 forward FFT on provided input vector.
 * @param input Real-valued samples of size `length`.
 */
void Fft::fft(const std::vector<float>& input)
{
    if (input.size() != length) return;
    memcpy(timeDomain, input.data(), length * sizeof(float));
    fft();
}

/**
 * Perform NE10 inverse FFT to produce time-domain data.
 */
void Fft::ifft()
{
    ne10_fft_c2r_1d_float32_neon(timeDomain, frequencyDomain, cfg);
}

/**
 * Perform NE10 inverse FFT using provided frequency-domain data.
 * @param reInput Real parts size `length/2+1`.
 * @param imInput Imag parts size `length/2+1`.
 */
void Fft::ifft(const std::vector<float>& reInput,
               const std::vector<float>& imInput)
{
    unsigned int half = length/2 + 1;
    if (reInput.size() < half || imInput.size() < half) return;
    for (unsigned i = 0; i < half; ++i) {
        fdr(i) = reInput[i];
        fdi(i) = imInput[i];
    }
    ifft();
}

#else
#include <cmath>

//--------------------------------------------------------------------------------
// FFTW-based implementation
//--------------------------------------------------------------------------------

/**
 * Check if n is a power of two.
 */
bool Fft::isPowerOfTwo(unsigned int n)
{
    if (n == 0) return false;
    while ((n & 1u) == 0u) n >>= 1;
    return n == 1;
}

/**
 * Round up n to the next power of two (minimum 2).
 */
unsigned int Fft::roundUpToPowerOfTwo(unsigned int n)
{
    if (n <= 2) return 2;
    unsigned int hb = 31 - __builtin_clz(n);
    if ((1u << hb) == n) return n;
    return (1u << (hb + 1));
}

/**
 * Initialize FFTW: allocate buffers and create forward/inverse plans.
 * @return 0 on success, -1 on failure.
 */
int Fft::setup(unsigned int len)
{
    if (!isPowerOfTwo(len))
        return -1;
    cleanup();
    length = len;
    timeIn       = (fftwf_complex*) fftwf_malloc(length * sizeof(fftwf_complex));
    freq         = (fftwf_complex*) fftwf_malloc(length * sizeof(fftwf_complex));
    timeOut      = (fftwf_complex*) fftwf_malloc(length * sizeof(fftwf_complex));
    forwardPlan  = fftwf_plan_dft_1d(length, timeIn,  freq,    FFTW_FORWARD,  FFTW_MEASURE);
    inversePlan  = fftwf_plan_dft_1d(length, freq,    timeOut, FFTW_BACKWARD, FFTW_MEASURE);
    if (!timeIn || !freq || !timeOut || !forwardPlan || !inversePlan) {
        cleanup();
        return -1;
    }
    return 0;
}

/**
 * Cleanup FFTW: destroy plans and free buffers.
 */
void Fft::cleanup()
{
    if (forwardPlan)  fftwf_destroy_plan(forwardPlan);
    if (inversePlan)  fftwf_destroy_plan(inversePlan);
    fftwf_free(timeIn);
    fftwf_free(freq);
    fftwf_free(timeOut);
    forwardPlan = inversePlan = nullptr;
    timeIn = freq = timeOut = nullptr;
    length = 0;
}

/**
 * Execute forward FFTW plan on internal buffers.
 */
void Fft::fft()
{
    fftwf_execute(forwardPlan);
}

/**
 * Execute forward FFTW on provided input vector.
 */
void Fft::fft(const std::vector<float>& input)
{
    if (input.size() != length) return;
    for (unsigned i = 0; i < length; ++i) {
        timeIn[i][0] = input[i];
        timeIn[i][1] = 0.0f;
    }
    fft();
}

/**
 * Execute inverse FFTW plan and normalize outputs.
 */
void Fft::ifft()
{
    fftwf_execute(inversePlan);
    for (unsigned i = 0; i < length; ++i) {
        timeOut[i][0] /= length;
        timeOut[i][1] /= length;
    }
}

/**
 * Execute inverse FFTW using provided real/imag vectors.
 */
void Fft::ifft(const std::vector<float>& reInput,
               const std::vector<float>& imInput)
{
    if (reInput.size() != length || imInput.size() != length) return;
    for (unsigned i = 0; i < length; ++i) {
        freq[i][0] = reInput[i];
        freq[i][1] = imInput[i];
    }
    ifft();
}
#endif
