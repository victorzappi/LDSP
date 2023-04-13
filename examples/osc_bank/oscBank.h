#pragma once

#include "wavetable.h"



class OscBank {


public:
	OscBank() {}										// Default constructor
					
	void setup(float sampleRate, int oscNum, float maxAmp,
               float minFreq, float maxFreq); 	// Set parameters
	
	float process();				// Get the next sample and update the phase
	
	~OscBank() {}				    // Destructor

private:
	std::vector<Wavetable> oscillators;	// bank osc oscillators, each is a wavetable
    float oscAmplitude; // all osc have same amp
};
