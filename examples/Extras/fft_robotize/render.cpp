// This code is based on the code credited below, but it has been modified
// further by Victor Zappi
/*
 ____  _____ _        _    
| __ )| ____| |      / \   
|  _ \|  _| | |     / _ \  
| |_) | |___| |___ / ___ \ 
|____/|_____|_____/_/   \_\

http://bela.io

C++ Real-Time Audio Programming with Bela - Lecture 18: Phase vocoder, part 1
fft-overlap-add-threads: overlap-add framework doing FFT in a low-priority thread
*/

#include <LDSP.h>
#include <libraries/Fft/Fft.h>
#include <vector>

// FFT-related variables
Fft gFft;					// FFT processing object
const int gFftSize = 1024;	// FFT window size in samples
int gHopSize = 256;			// How often we calculate a window

// Circular buffer and pointer for assembling a window of samples
const int gBufferSize = 16384;
std::vector<float> gInputBuffer;
int gInputBufferPointer = 0;
int gHopCounter = 0;

// Circular buffer for collecting the output of the overlap-add process
std::vector<float> gOutputBuffer;
int gOutputBufferWritePointer = gFftSize + gHopSize;
int gOutputBufferReadPointer = 0;

// Buffer to hold the windows for FFT analysis and synthesis
std::vector<float> gWindowBuffer;

float gGain = 4.0;


bool setup(LDSPcontext *context, void *userData)
{
	// Set up the FFT and its buffers
	gFft.setup(gFftSize);
	gInputBuffer.resize(gBufferSize);
	gOutputBuffer.resize(gBufferSize);
	
	// Calculate the window
	gWindowBuffer.resize(gFftSize);
	for(int n = 0; n < gFftSize; n++) 
		gWindowBuffer[n] = 0.5f * (1.0f - cosf(2.0 * M_PI * n / (float)(gFftSize - 1))); // Hann window
	
	return true;
}

// This function handles the FFT processing in this example once the buffer has
// been assembled.
void process_fft(std::vector<float> const& inBuffer, unsigned int inPointer, std::vector<float>& outBuffer, unsigned int outPointer)
{
	static std::vector<float> unwrappedBuffer(gFftSize);	// Container to hold the unwrapped values
	
	// Copy buffer into FFT input
	for(int n = 0; n < gFftSize; n++) 
	{
		// Use modulo arithmetic to calculate the circular buffer index
		int circularBufferIndex = (inPointer + n - gFftSize + gBufferSize) % gBufferSize;
		unwrappedBuffer[n] = inBuffer[circularBufferIndex] * gWindowBuffer[n];
	}
	
	// Process the FFT based on the time domain input
	gFft.fft(unwrappedBuffer);
		
	// Robotise the output
	for(int n = 0; n < gFftSize; n++) 
	{
		float amplitude = gFft.fda(n);
		gFft.fdr(n) = amplitude;
		gFft.fdi(n) = 0;
	}
		
	// Run the inverse FFT
	gFft.ifft();
	
	// Add timeDomainOut into the output buffer
	for(int n = 0; n < gFftSize; n++) 
	{
		int circularBufferIndex = (outPointer + n - gFftSize + gBufferSize) % gBufferSize;
		outBuffer[circularBufferIndex] += gFft.td(n) * gWindowBuffer[n];
	}
}


void render(LDSPcontext *context, void *userData)
{
	for(unsigned int n = 0; n < context->audioFrames; n++) 
	{
        // Read the next sample from the buffer
        float in = audioRead(context, n, 0);

		// Store the sample ("in") in a buffer for the FFT
		// Increment the pointer and when full window has been 
		// assembled, call process_fft()
		gInputBuffer[gInputBufferPointer++] = in;
		if(gInputBufferPointer >= gBufferSize) 
		{
			// Wrap the circular buffer
			// Notice: this is not the condition for starting a new FFT
			gInputBufferPointer = 0;
		}
		
		// Get the output sample from the output buffer
		float out = gOutputBuffer[gOutputBufferReadPointer];
		
		// Then clear the output sample in the buffer so it is ready for the next overlap-add
		gOutputBuffer[gOutputBufferReadPointer] = 0;
		
		// Scale the output down by the overlap factor (e.g. how many windows overlap per sample?)
		out *= (float)gHopSize / (float)gFftSize;
		
		// Increment the read pointer in the output cicular buffer
		gOutputBufferReadPointer++;
		if(gOutputBufferReadPointer >= gBufferSize)
			gOutputBufferReadPointer = 0;
		
		// Increment the hop counter and start a new FFT if we've reached the hop size
		if(++gHopCounter >= gHopSize) 
		{
			gHopCounter = 0;
			
			process_fft(gInputBuffer, gInputBufferPointer, gOutputBuffer, gOutputBufferWritePointer);
			// Update the output buffer write pointer to start at the next hop
			gOutputBufferWritePointer = (gOutputBufferWritePointer + gHopSize) % gBufferSize;
		}

		out *= gGain;
		// Write the audio to the output
		audioWrite(context, n, 0, out);
		audioWrite(context, n, 1, out);
	}
}

void cleanup(LDSPcontext *context, void *userData)
{

}
