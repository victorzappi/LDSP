#include "Fft.h"
#include <math.h>
#include <string.h>



bool Fft::setup(unsigned int length) {

    this->length = length;

    this->timeIn = (fftwf_complex *) fftwf_malloc(length * sizeof(fftwf_complex));
    this->timeOut = (fftwf_complex *) fftwf_malloc(length * sizeof(fftwf_complex));
    this->freq = (fftwf_complex *) fftwf_malloc(length * sizeof(fftwf_complex));
    this->forwardPlan = fftwf_plan_dft_1d(this->length, this->timeIn, this->freq, FFTW_FORWARD, FFTW_MEASURE);
    this->inversePlan = fftwf_plan_dft_1d(this->length, this->freq, this->timeOut, FFTW_BACKWARD, FFTW_MEASURE);

    return true;
}

void Fft::cleanup() {
    fftwf_free(this->timeIn);
    fftwf_free(this->timeOut);
    fftwf_free(this->freq);
    fftwf_destroy_plan(forwardPlan);
    fftwf_destroy_plan(inversePlan);
    fftwf_cleanup();
}


void Fft::fft() {
    fftwf_execute(forwardPlan);
}

void Fft::fft(const std::vector<float> &input) {
    if (input.size() > this->length) {
        return;
    }
    for (int i = 0; i < this->length; i++) {
        this->timeIn[i][FFT_REAL] = input[i];
        this->timeIn[i][FFT_IMAG] = 0.0f;
    }
    fft();
}

void Fft::ifft() {
    fftwf_execute(inversePlan);
    for (int i = 0; i < this->length; i++) {
        this->timeOut[i][FFT_REAL] /= this->length;
        this->timeOut[i][FFT_IMAG] /= this->length;
    }
}

void Fft::ifft(const std::vector<float> &real, const std::vector<float> &imag) {
    if (real.size() > this->length || imag.size() > this->length) {
        return;
    }
    for (int i = 0; i < this->length; i++) {
        this->freq[i][FFT_REAL] = real[i];
        this->freq[i][FFT_IMAG] = imag[i];
    }
    ifft();
}
