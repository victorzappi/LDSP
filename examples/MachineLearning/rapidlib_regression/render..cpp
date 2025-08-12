/*
 * LDSP 2D Touch to Timbre Regression using RapidLib
 * Maps 2D touch positions to filter parameters
 * Simple workflow: generate random timbres, map to positions, then interact
 */

#include "LDSP.h"
#include <rapidLib.h>
#include "libraries/Biquad/Biquad.h"
#include <vector>
#include <cmath>
#include <iostream>
#include <cstdlib>

// User settings
float baseFrequency = 110.0f;           // Base frequency of sawtooth
float baseAmplitude = 0.2f;             // Output amplitude
float parameterSmoothingFactor = 0.25f; // Fast parameter smoothing for responsive control

float maxRandomCut = 3200;
float minRandomCut = 200;
float maxRandomQ = 15.5;
float minRandomQ = 0.5;

// Keep these ranges larger than the random ones, to have more regression fun!
float maxRegressedCut = 8000;
float minRegressedCut = 150;
float maxRegressedQ = 20;
float minRegressedQ = 0.4;


const int wavetableLength = 512;
const int numHarmonics = 20;  // Limit to prevent aliasing

//------------------------------------------------

// Wavetable
float wavetable[wavetableLength];
float readPointer = 0.0f;

// RapidLib regression
rapidLib::regression regressor;

// FSM States
enum State
{
	IDLE,
	MAPPING,
	INTERACTIVE
};

State currentState = IDLE;

// Training data
std::vector<rapidLib::trainingExample> trainingSet;
int mappingCount = 0;
const int MAPPINGS_NEEDED = 4;

// Current timbre parameters
float currentCutoff = 1000.0f;
float currentQ = 0.707f;
float targetFilterCutoff = 1000.0f;
float targetFilterQ = 0.707f;

// Button states for edge detection
bool volumeUpPressed = false;
bool volumeDownPressed = false;
bool prevVolumeUp = false;
bool prevVolumeDown = false;

// Touch tracking
float maxTouchX;
float maxTouchY;
bool touchActive = false;
bool prevTouchActive = false;
float touchX = 0.0f;
float touchY = 0.0f;

// Audio synthesis
float amplitude = 0.0f;
float targetAmplitude = 0.0f;
Biquad filter;

//------------------------------------------------

// Generate random timbre parameters
void generateRandomTimbre()
{
	// Generate interesting random filter settings
    targetFilterCutoff = minRandomCut + ((float)rand() / (float)RAND_MAX) * (maxRandomCut-minRandomCut);
	targetFilterQ = minRandomQ + ((float)rand() / (float)RAND_MAX) * (maxRandomQ - minRandomQ);
	
	std::cout << "New timbre: Cutoff=" << targetFilterCutoff 
	         << " Hz, Q=" << targetFilterQ << std::endl;
}

// Initialize wavetable with band-limited sawtooth (additive synthesis)
void initializeWavetable()
{
	// Clear the wavetable first
	for(int n = 0; n < wavetableLength; n++)
		wavetable[n] = 0.0f;
	
	// Add harmonics (sawtooth has all harmonics with amplitude 1/n)
	for(int h = 1; h <= numHarmonics; h++)
	{
		float harmonicAmp = 1.0f / (float)h;  // Sawtooth amplitude pattern
		for(int n = 0; n < wavetableLength; n++)
		{
			float phase = 2.0f * M_PI * (float)n / (float)wavetableLength;
			wavetable[n] += harmonicAmp * sinf(h * phase);
		}
	}
	
	// Normalize to [-1, 1] range
	float maxVal = 0.0f;
	for(int n = 0; n < wavetableLength; n++)
		if(fabs(wavetable[n]) > maxVal)
			maxVal = fabs(wavetable[n]);
	
	for(int n = 0; n < wavetableLength; n++)
		wavetable[n] /= maxVal;
}

bool setup(LDSPcontext *context, void *userData)
{
	readPointer = 0.0f;
	currentState = IDLE;
	mappingCount = 0;
	
	// Seed random number generator
	srand(time(NULL));
	
    initializeWavetable();

	for(int n = 0; n < wavetableLength; n++)
		wavetable[n] = -1.0f + 2.0f * (float)n / (float)wavetableLength;
	
	Biquad::Settings settings{
		.fs = context->audioSampleRate,
		.type = Biquad::lowpass,
		.cutoff = 1000,
		.q = 0.707,
		.peakGainDb = 0,
	};
	filter.setup(settings);
	
	maxTouchX = context->mtInfo->screenResolution[0] - 1;
	maxTouchY = context->mtInfo->screenResolution[1] - 1;
	
	std::cout << "\n=== LDSP 2D Touch to Timbre Mapping ===" << std::endl;
	std::cout << "Map 4 random timbres to touch positions" << std::endl;
	std::cout << "\nControls:" << std::endl;
	std::cout << "  Volume UP: Start mapping / Generate new timbre" << std::endl;
	std::cout << "  Touch: Map current timbre to position" << std::endl;
	std::cout << "  Volume DOWN: Reset to idle" << std::endl;
	std::cout << "\nState: IDLE (silent)" << std::endl;
	std::cout << "Press Volume UP to begin\n" << std::endl;
	
	return true;
}

void render(LDSPcontext *context, void *userData)
{
	// Read button states
	volumeUpPressed = buttonRead(context, chn_btn_volUp);
	volumeDownPressed = buttonRead(context, chn_btn_volDown);
	
	// Detect button edges
	bool volumeUpTrigger = volumeUpPressed && !prevVolumeUp;
	bool volumeDownTrigger = volumeDownPressed && !prevVolumeDown;
	
	// Read touch input
	if(context->mtInfo->anyTouchSupported)
		touchActive = (multiTouchRead(context, chn_mt_anyTouch) == 1);
	else
		touchActive = (multiTouchRead(context, chn_mt_id, 0) != -1);
	
	if(touchActive)
	{
		touchX = multiTouchRead(context, chn_mt_x, 0) / maxTouchX;
		touchY = multiTouchRead(context, chn_mt_y, 0) / maxTouchY;
	}
	
	// Detect touch trigger (rising edge)
	bool touchTrigger = touchActive && !prevTouchActive;
	
	// FSM Logic
	switch(currentState)
	{
		case IDLE:
			targetAmplitude = 0.0f;
			
			if(volumeUpTrigger)
			{
				currentState = MAPPING;
				targetAmplitude = baseAmplitude;
				generateRandomTimbre();
				mappingCount = 0;
				trainingSet.clear();
				std::cout << "\nState: MAPPING" << std::endl;
				std::cout << "Touch screen to map this timbre (need " 
				         << MAPPINGS_NEEDED << " mappings)" << std::endl;
			}
			
			if(volumeDownTrigger)
				std::cout << "Already in IDLE" << std::endl;
			break;
			
		case MAPPING:
			targetAmplitude = baseAmplitude;
			
			if(touchTrigger)
			{
				// Map current timbre to touch position
				rapidLib::trainingExample example;
				example.input = {(double)touchX, (double)touchY};
				example.output = {targetFilterCutoff, targetFilterQ};
				trainingSet.push_back(example);
				mappingCount++;
				
				std::cout << "Mapped " << mappingCount << "/" << MAPPINGS_NEEDED 
				         << " at position [" << touchX << ", " << touchY << "]" << std::endl;
				
				if(mappingCount >= MAPPINGS_NEEDED)
				{
					// Train and switch to interactive mode
					bool success = regressor.train(trainingSet);
					if(success)
					{
						currentState = INTERACTIVE;
						std::cout << "\n*** Training successful! ***" << std::endl;
						std::cout << "State: INTERACTIVE" << std::endl;
						std::cout << "Touch anywhere to control timbre" << std::endl;
						std::cout << "Volume DOWN to reset\n" << std::endl;
					}
					else
					{
						std::cout << "!!! Training failed! Resetting..." << std::endl;
						currentState = IDLE;
						targetAmplitude = 0.0f;
						mappingCount = 0;
						trainingSet.clear();
					}
				}
				else
				{
					// Generate new random timbre for next mapping
					generateRandomTimbre();
					std::cout << "Touch screen to map this new timbre\n" << std::endl;
				}
			}
			else if(volumeUpTrigger)
			{
				// Generate new random timbre without mapping
				generateRandomTimbre();
				std::cout << "Skipped previous timbre, try this one\n" << std::endl;
			}
			else if(volumeDownTrigger)
			{
				// Reset to idle
				currentState = IDLE;
				targetAmplitude = 0.0f;
				mappingCount = 0;
				trainingSet.clear();
				std::cout << "\n*** RESET ***" << std::endl;
				std::cout << "State: IDLE (silent)" << std::endl;
				std::cout << "Press Volume UP to begin\n" << std::endl;
			}
			break;
			
		case INTERACTIVE:
			// Sound only when touching
			targetAmplitude = touchActive ? baseAmplitude : 0.0f;
			
			// Run regression on current touch position
			if(touchActive)
			{
				std::vector<double> input = {(double)touchX, (double)touchY};
				std::vector<double> output = regressor.run(input);
				
				if(output.size() >= 2)
				{
					targetFilterCutoff = output[0];
					targetFilterQ = output[1];
					
					// Clamp to reasonable ranges
					if(targetFilterCutoff < minRegressedCut) targetFilterCutoff = minRegressedCut;
					if(targetFilterCutoff > maxRegressedCut) targetFilterCutoff = maxRegressedCut;
					if(targetFilterQ < minRegressedQ) targetFilterQ = minRegressedQ;
					if(targetFilterQ > maxRegressedQ) targetFilterQ = maxRegressedQ;
				}
			}
			// When touch stops, keep last regression output (no change needed)
			
			if(volumeDownTrigger)
			{
				// Reset to idle
				currentState = IDLE;
				targetAmplitude = 0.0f;
				mappingCount = 0;
				trainingSet.clear();
				std::cout << "\n*** RESET ***" << std::endl;
				std::cout << "State: IDLE (silent)" << std::endl;
				std::cout << "Press Volume UP to begin\n" << std::endl;
			}
			break;
	}
	
	// Update button and touch history
	prevVolumeUp = volumeUpPressed;
	prevVolumeDown = volumeDownPressed;
	prevTouchActive = touchActive;
	
	// Smooth parameter changes
	amplitude = amplitude * 0.95f + targetAmplitude * 0.05f;
	currentCutoff = currentCutoff * (1.0f - parameterSmoothingFactor) + targetFilterCutoff * parameterSmoothingFactor;
	currentQ = currentQ * (1.0f - parameterSmoothingFactor) + targetFilterQ * parameterSmoothingFactor;
	
	// Update filter settings
	Biquad::Settings filterSettings{
		.fs = context->audioSampleRate,
		.type = Biquad::lowpass,
		.cutoff = currentCutoff,
		.q = currentQ,
		.peakGainDb = 0,
	};
	filter.setup(filterSettings);
	
	// Generate audio with wavetable synthesis
	for(int n = 0; n < context->audioFrames; n++)
	{
		// Linear interpolation in wavetable
		int indexBelow = floorf(readPointer);
		int indexAbove = (indexBelow + 1) % wavetableLength;
		float fraction = readPointer - indexBelow;
		
		float sample = wavetable[indexBelow] * (1 - fraction) + wavetable[indexAbove] * fraction;
		sample *= amplitude;
		
		// Apply filter
		float out = filter.process(sample);
		
		// Update read pointer
		readPointer += wavetableLength * baseFrequency / context->audioSampleRate;
		while(readPointer >= wavetableLength)
			readPointer -= wavetableLength;
		
		// Write to all output channels
		for(int chn = 0; chn < context->audioOutChannels; chn++)
			audioWrite(context, n, chn, out);
	}
}

void cleanup(LDSPcontext *context, void *userData)
{
	std::cout << "LDSP 2D Touch Timbre Cleanup" << std::endl;
	trainingSet.clear();
}