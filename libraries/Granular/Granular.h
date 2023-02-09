/**
 * Author: Jason Hoopes
 */

#include "LDSP.h"
#include <vector>
#include <string>
#include <libraries/AudioFile/AudioFile.h>
#include <cmath>

#pragma once



class Granular 
{

public:

	Granular();
	~Granular();
	
	/**
	 * Setup all parameters necessary for granulation
	 *
	 * @param numGrains - number of grains to use during playback
	 * @param sampleRate - sample rate of audio device
	 * @param filename - String containing path to wav file (relative to render.cpp)

	 */
	bool setup(int numGrains, float sampleRate, const std::string& filename);
	
	
	/**
	 * Process the next audio frame from the grain at the given index
	 * 
	 * @param grainIdx - index of grain to process
	 * 
	 * @return an amplitude value between 0.0 and 1.0
	 */
	float processGrain(int grainIdx);

	/**
	 * Process the next audio frame, summed over all active grains
	 */
	float process();

	/**
	 * Generates a gain value between 0.0 and 1.0, based on a window of the given grain size 
	 * 
	 * @param pointer - the position of the current pointer
	 * @param grainSize - the size of the grain that is being windowed
	 * 
	 * @return a gain value normalized between 0.0 and 1.0
	 */
	float applyWindow(int pointer, float grainSize);
	
	/**
	 * Initializes every parameter to its default value
	 */
	bool initializeAllParams(float sampleRate);
	
	// Cubic interpolation 
	/**
	 * Interpolates the value of a sample with position `pos` between frames f1 and f2. 
	 * f0 and f3 are the neighboring samples on each respective side
	 * 
	 * @param pos - the position between frames f1 and f2
	 * 
	 * @returns the interpolated amplitude value
	 */ 
	float interpolateCubic(float f0, float f1, float f2, float f3, float pos);


	// Set grain size in milliseconds 
	void setGrainSizeMS(float grainSize);
	// Set file position in milliseconds
	void setFilePositionMS(float filePosition);
	//VIC
	// Set file normalized position, where 0 is start and 1 is end of file
	void setFilePosition(float filePosition);
	// Set speed of read pointers
	// Playback speed is the amount of samples that the read pointer will be incremented by at each audio frame
	void setPlaybackSpeed(float playbackSpeed);
	
	// Set the probability that a grain will be played 
	// This must be a value between 0.0 and 1.0
	void setProbability(float probability);
	// Set the amount of randomness (0.0-1.0) that will be applied to the grain size
	void setGrainSizeRandomness(float randomness);
	// Set the amount of randomness (0.0-1.0) that will be applied to the file position;
	void setFilePositionRandomness(float randomness);
	
	// Propogates the global, stored parameters to a single grain
	void updateGrain(float grainIdx);
	// Propogates the global, stored parameters to all grains
	void updateAllGrains();
	
private:

	// Number of Grains
	int _numGrains;
	
	// Sample Rate for calculations
	float _sampleRate;
	
	// Buffer of sample values that will be read by grains
	std::vector<float> _sampleBuffer;

	// The current read pointer for each respective grain in frames
	std::vector<float> _pointers;
	
	// The current starting position of each grain in frames
	std::vector<float> _filePositions;
	
	// The current size of each grain in frames
	std::vector<float> _grainSizes;
	
	// Whether each grain should be muted for its next cycle. 
	// This resets at the end of every grain's cycle
	std::vector<bool> _muted;


	float _filePosition;
	float _grainSize;
	float _playbackSpeed;
	float _probability;
	float _filePositionRandomness;
	float _grainSizeRandomness;

	//VIC
	float _lastSample;
	std::vector<float> _raisedCosine;
	int _cosineResolution;
};


inline float Granular::interpolateCubic(float f0, float f1, float f2, float f3, float pos) {
	
	float a0 = f3 - f2 - f0 + f1;
	float a1 = f0 - f1 - a0;
	float a2 = f2 - f0;
	float a3 = f1;

	return a0*(std::pow(pos,3)) + a1*(std::pow(pos,2)) + a2*pos + a3;
}

inline float Granular::applyWindow(int pointer, float grainSize) {
	
	int windowType = 0; 
	if (grainSize < 1) {
		return 0;
	}

	if (windowType == 1) {
		return map((float) pointer, 0.0, grainSize, 1.0, 0.0);
	}
	
	else if (windowType == 2) {
		return map((float) pointer, 0.0, grainSize, 0.0, 1.0);
	}
	
	else { //VIC changed to raised cosine
		//return -( 1-cosf( 2*M_PI*pointer/grainSize) ) / 2.0; //sinf((M_PI*pointer)/grainSize);
		int n = this->_cosineResolution*(pointer/grainSize);
		return this->_raisedCosine[n];
	}
	
}

inline void Granular::updateGrain(float grainIdx) {

	_grainSizes[grainIdx] = _grainSize;
	_filePositions[grainIdx] = _filePosition;
	
	// _muted[grainIdx] = rand()%100 > _probability;
	
	float random = rand()%100;
	_grainSizes[grainIdx] -= (random/100.0*_grainSizeRandomness*_grainSize);
	//_filePositions[grainIdx] += (random/100.0*_filePositionRandomness*_filePosition);
	_filePositions[grainIdx] += (random/100.0*_filePositionRandomness*_lastSample); //VIC random factor across file size
}






