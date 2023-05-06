#include "LDSP.h"
#include <vector>
#include "MonoFilePlayer.h"
#include <fft.h>


string filename = "323623__shpira__tech-drums.wav"; // https://freesound.org/s/323623/

MonoFilePlayer player;


FFT fft;
int n_fft = 1024;
int nyquist;

vector<float> inputBuffer;
vector<float> outputBuffer;
vector<float> real;
vector<float> imag;

int writePointer = 0;
int readPointer = 0;

float energyLow = 0.0;
float energyHigh = 0.0;
float energyThreshold = 0.5;

float stateLow = 0.0;
float stateHigh = 0.0;

float vibrationDuration = 100.0;



bool setup(LDSPcontext *context, void *userData)
{
	// set screen on and keep it on, otherwise touches are not registered
	screenSetState(true, 1, true);

 	// Load the audio file
	if(!player.setup(filename)) {
    	printf("Error loading audio file '%s'\n", filename.c_str());
    	return false;
	}

	// Setup the FFT object
	if(!fft.setup(n_fft)) {
		printf("Error setting up FFT with length %d\n", n_fft);
		return false;
	}

	nyquist = n_fft / 2;
	inputBuffer.resize(n_fft);

    return true;
}


void processFFT(LDSPcontext *context) {

	fft.fft(inputBuffer);

	int upperBand = nyquist / 16;
	for (int i = 0; i < nyquist; i++) {
		if (i < upperBand) {
			energyLow += fft.getRealComponent(i);
		}
		else {
			energyHigh += fft.getRealComponent(i);
		}
	}
	// Normalize energy
	energyLow /= upperBand;

	if (energyLow > energyThreshold) 
		ctrlOutputWrite(context, chn_cout_vibration, vibrationDuration);

}


void render(LDSPcontext *context, void *userData)
{	

	for(int n=0; n<context->audioFrames; n++)
	{

		float in = player.process(); 
		inputBuffer[writePointer] = in;
		writePointer++;
		// perform an fft once we have filled enough samples into the buffer
		if (writePointer > n_fft) {
			processFFT(context);
			writePointer = 0;
		}

		for(int chn=0; chn<context->audioOutChannels; chn++)
			audioWrite(context, n, chn, in);	
	}

}

void cleanup(LDSPcontext *context, void *userData)
{

}