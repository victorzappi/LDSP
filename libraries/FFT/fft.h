#pragma once

#include <fftw3.h>
#include <vector>


#define FFT_REAL 0
#define FFT_IMAG 1


class FFT {
public:
    FFT() {};
    ~FFT() { cleanup();} ;


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

private:
    /* Length of the FFT*/
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