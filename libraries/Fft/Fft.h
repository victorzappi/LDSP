#pragma once

#include <fftw3.h>
#include <vector>


#define FFT_REAL 0
#define FFT_IMAG 1


class Fft {
public:
    Fft() {};
    ~Fft() { cleanup();} ;


    void cleanup();
    bool setup(unsigned int length);

    void fft();
    void fft(const std::vector<float> &input);

    void ifft();
    void ifft(const std::vector<float> &real, const std::vector<float> &imag);

    // Fills a pointer with the real part of frequency data
    void getReal(float * real);
    float getRealComponent(int idx);

    // Fills a pointer with the imaginary part of frequency data
    void getImag(float * imag);
    float getImagComponent(int idx);

    // Fills a pointer with the time domain signal
    void getTimeDomain(float * samples);
    float getTimeDomainSample(int idx);

    // Methods for compatibility with Bela'sFft
    float& fdr(unsigned int n);
	/**
	 * Get the imaginary part of the frequency-domain representation at index `n`.
	 */
	float& fdi(unsigned int n);
	/**
	 * Get the absolute value of the frequency-domain representation at index `n`.
	 * The value is computed on the fly at each call and is not cached.
	 */
	float fda(unsigned int n);
    /**
	 * Get the time-domain representation at index `n`.
	 */
	float& td(unsigned int n);

private:
    /* Length of the Fft*/
    unsigned int length;
    /* Holds the input to the forward transform */
    fftwf_complex * timeIn;
    /* Holds the Frequency Domain data*/
    fftwf_complex * freq;
    /* Holds the result of the inverse transform*/
    fftwf_complex * timeOut;

    fftwf_plan forwardPlan;
    fftwf_plan inversePlan;
};

//----------------------------------------------------------

inline void Fft::getReal(float * real) {
    for (int i = 0; i < length; i++) {
        real[i] = this->freq[i][FFT_REAL];
    }
}

inline float Fft::getRealComponent(int idx) {
    return this->freq[idx][FFT_REAL];
}

inline void Fft::getImag(float * imag) {
    for (int i = 0; i < length; i++) {
        imag[i] = this->freq[i][FFT_IMAG];
    }
}

inline float Fft::getImagComponent(int idx) {
    return this->freq[idx][FFT_IMAG];
}

inline void Fft::getTimeDomain(float * samples) {
    for (int i = 0; i < length; i++) {
        samples[i] = this->timeOut[i][FFT_REAL];
    }
}

inline float Fft::getTimeDomainSample(int idx) {
    return this->timeOut[idx][FFT_REAL];
}

inline float& Fft::fdr(unsigned int n) { 
    return this->freq[n][FFT_REAL];
};

inline float& Fft::fdi(unsigned int n) { 
    return this->freq[n][FFT_IMAG];    
};

inline float Fft::fda(unsigned int n) { 
    return sqrtf(fdr(n) * fdr(n) + fdi(n) * fdi(n)); 
};

inline float& Fft::td(unsigned int n) { 
    return timeOut[n][FFT_REAL]; 
};