#include "fft.h"
#include <math.h>
#include <string.h>



bool FFT::setup(unsigned int length) {

    this->length = length;

    this->timeIn = (fftwf_complex *) fftwf_malloc(length * sizeof(fftwf_complex));
    this->timeOut = (fftwf_complex *) fftwf_malloc(length * sizeof(fftwf_complex));
    this->freq = (fftwf_complex *) fftwf_malloc(length * sizeof(fftwf_complex));
    this->forwardPlan = fftwf_plan_dft_1d(this->length, this->timeIn, this->freq, FFTW_FORWARD, FFTW_MEASURE);
    this->inversePlan = fftwf_plan_dft_1d(this->length, this->freq, this->timeOut, FFTW_BACKWARD, FFTW_MEASURE);

    return true;
}

void FFT::cleanup() {
    fftwf_free(this->timeIn);
    fftwf_free(this->timeOut);
    fftwf_free(this->freq);
    fftwf_destroy_plan(forwardPlan);
    fftwf_destroy_plan(inversePlan);
    fftwf_cleanup();
}


void FFT::fft() {
    fftwf_execute(forwardPlan);
}

void FFT::fft(const std::vector<float> &input) {
    if (input.size() > this->length) {
        return;
    }
    for (int i = 0; i < this->length; i++) {
        this->timeIn[i][FFT_REAL] = input[i];
        this->timeIn[i][FFT_IMAG] = 0.0f;
    }
    fft();
}

void FFT::ifft() {
    fftwf_execute(inversePlan);
    for (int i = 0; i < this->length; i++) {
        this->timeOut[i][FFT_REAL] /= this->length;
        this->timeOut[i][FFT_IMAG] /= this->length;
    }
}

void FFT::ifft(const std::vector<float> &real, const std::vector<float> &imag) {
    if (real.size() > this->length || imag.size() > this->length) {
        return;
    }
    for (int i = 0; i < this->length; i++) {
        this->freq[i][FFT_REAL] = real[i];
        this->freq[i][FFT_IMAG] = imag[i];
    }
    ifft();
}

void FFT::getReal(float * real) {
    for (int i = 0; i < length; i++) {
        real[i] = this->freq[i][FFT_REAL];
    }
}

float FFT::getRealComponent(int idx) {
    return this->freq[idx][FFT_REAL];
}

void FFT::getImag(float * imag) {
    for (int i = 0; i < length; i++) {
        imag[i] = this->freq[i][FFT_IMAG];
    }
}

float FFT::getImagComponent(int idx) {
    return this->freq[idx][FFT_IMAG];
}

void FFT::getTimeDomain(float * samples) {
    for (int i = 0; i < length; i++) {
        samples[i] = this->timeOut[i][FFT_REAL];
    }
}

float FFT::getTimeDomainSample(int idx) {
    return this->timeOut[idx][FFT_REAL];
}
