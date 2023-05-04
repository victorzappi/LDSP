#include <LDSP.h>
#include <fft.h>
#include <iostream>


FFT fft;

std::vector<float> fft_in;
std::vector<float> real;
std::vector<float> imag;

int n_fft;

bool setup(LDSPcontext *context, void *userData)
{
    
    n_fft = context->audioFrames;

    if (!fft.setup(n_fft)) {
        std::cout << "error initializing FFT object" << std::endl;
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
    for (int i = 0; i < n_fft; i++) {
        sum = 0.0;
        for (int channel = 0; channel < context->audioInChannels; channel++) {
            sum += audioRead(context, i, channel);
        }
        fft_in[i] = sum / (float) context->audioInChannels;
    }

    // Perform the FFT
    fft.fft(fft_in);


    // ... Do something with the Spectral Data...
    for (int i = 0; i < n_fft; i++) {
        real[i] = fft.getRealComponent(i) * 0;
        imag[i] = fft.getImagComponent(i) * 0;
    }


    // Perform the Inverse Transform
    fft.ifft(real, imag);

    for (int i = 0; i < n_fft; i++) {
        for (int channel = 0; channel < context->audioOutChannels; channel++) {
            audioWrite(context, i, channel, fft.getTimeDomainSample(i));
        }
    }  
}

void cleanup(LDSPcontext *context, void *userData)
{
}