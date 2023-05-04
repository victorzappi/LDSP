#include "fft.h"
#include <math.h>
#include <string.h>



bool FFT::setup(unsigned int length) {

    this->length = length;

    this->time = (fftw_complex *) fftw_malloc(length * sizeof(fftw_complex));
    this->freq = (fftw_complex *) fftw_malloc(length * sizeof(fftw_complex));
    this->forward_plan = fftw_plan_dft_1d(this->length, this->time, this->freq, FFTW_FORWARD, FFTW_MEASURE);
    this->inverse_plan = fftw_plan_dft_1d(this->length, this->freq, this->time, FFTW_BACKWARD, FFTW_MEASURE);

    return true;
}

void FFT::cleanup() {
    fftw_free(this->time);
    fftw_free(this->freq);
    fftw_destroy_plan(forward_plan);
    fftw_destroy_plan(inverse_plan);
    fftw_cleanup();
}


void FFT::fft() {
    fftw_execute(forward_plan);
}

void FFT::fft(const std::vector<float> &input) {
    if (input.size() > this->length) {
        return;
    }
    for (int i = 0; i < this->length; i++) {
        this->time[i][FFT_REAL] = input[i];
        this->time[i][FFT_IMAG] = 0.0f;
    }
    fft();
}

void FFT::ifft() {
    fftw_execute(inverse_plan);
    for (int i = 0; i < this->length; i++) {
        this->time[i][FFT_REAL] /= this->length;
        this->time[i][FFT_IMAG] /= this->length;
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
        samples[i] = this->time[i][FFT_REAL];
    }
}

float FFT::getTimeDomainSample(int idx) {
    return this->time[idx][FFT_REAL];
}
