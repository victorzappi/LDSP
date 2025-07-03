#pragma once
#include <vector>
#ifdef NE10_FFT
#include <NE10.h>
#include <cmath> // #include <libraries/math_neon/math_neon.h>  //VIC math_neon not yet available
#else
#include <fftw3.h>
#include <cmath>
#endif

/**
 * A real-to-complex FFT and complex-to-real IFFT. This is a wrapper for the FFTW library.
*/

class Fft
{
public:
    Fft() = default;
    Fft(unsigned int length) { setup(length); };
    ~Fft() { cleanup(); };

    /**
     * Initialize buffers and plans for a transform of size `length`.
     */
    int setup(unsigned int length);

    void cleanup();

    /**
     * Perform the FFT of the signal whose time-domain representation is stored internally.
     */
    void fft();

    /**
     * Perform the FFT of the signal whose time-domain representation is passed as an argument.
     */
    void fft(const std::vector<float>& input);

    /**
     * Perform the IFFT of the internal frequency-domain signal.
     */
    void ifft();

    /**
     * Perform the IFFT of the signal whose frequency-domain representation is passed as arguments.
     */
    void ifft(const std::vector<float>& reInput, const std::vector<float>& imInput);

    /**
     * Get the real part of the frequency-domain representation at index `n`.
     */
    float& fdr(unsigned int n) 
    { 
    #ifdef NE10_FFT
        return frequencyDomain[n].r;
    #else
        return frequencyDomain[n][0]; 
    #endif
    };
    /**
     * Get the imaginary part of the frequency-domain representation at index `n`.
     */
    float& fdi(unsigned int n) { 
    #ifdef NE10_FFT
        return frequencyDomain[n].i;
    #else
        return frequencyDomain[n][1]; 
    #endif    
    };
    /**
     * Get the absolute value of the frequency-domain representation at index `n`.
     * The value is computed on the fly at each call and is not cached.
     */
    float fda(unsigned int n) 
    { 
    #ifdef NE10_FFT
        return std::sqrt(fdr(n) * fdr(n) + fdi(n) * fdi(n)); //sqrtf_neon(fdr(n) * fdr(n) + fdi(n) * fdi(n)); //VIC this will be turned into math_neon implementation
    #else
        return std::sqrt(fdr(n) * fdr(n) + fdi(n) * fdi(n)); 
    #endif
    };
    /**
     * Get the time-domain representation at index `n`.
     */
    float& td(unsigned int n)
    { 
    #ifdef NE10_FFT
        return timeDomain[n];
    #else
        return timeDomain[n][0]; 
    #endif    
        
    };

    static bool isPowerOfTwo(unsigned int n);
    static unsigned int roundUpToPowerOfTwo(unsigned int n);
private:
    unsigned int length;
#ifdef NE10_FFT
   	ne10_float32_t* timeDomain = nullptr;
	ne10_fft_cpx_float32_t* frequencyDomain = nullptr;
	ne10_fft_r2c_cfg_float32_t cfg = nullptr; 
#else
    fftwf_complex* timeIn;
    fftwf_complex* frequencyDomain;
    fftwf_complex* timeDomain;
    fftwf_plan forwardPlan;
    fftwf_plan inversePlan;
#endif
};
