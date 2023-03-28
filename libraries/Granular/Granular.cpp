/**
 * Author: Jason Hoopes
 */ 


#include "Granular.h"

#define COS_RESOLUTION 512

Granular::Granular() {}
Granular::~Granular() {}

bool Granular::initializeAllParams(float sampleRate) 
{

	for (int i = 0; i < _numGrains; i++) {
		
		this->_pointers[i] = 0.0;
		this->_filePositions[i] = 0;
		this->_grainSizes[i] = 0.0 * sampleRate;
		this->_muted[i] = false;
		
	}

	//VIC
	this->_cosineResolution = COS_RESOLUTION;
	this->_raisedCosine.resize(this->_cosineResolution);
	float size = this->_raisedCosine.size();
	for(int n=0; n<size; n++)
		this->_raisedCosine[n] = -( 1-cosf( 2*M_PI*n/(size-1)) ) / 2.0;
	
	return true;
}

bool Granular::setup(int numGrains, float sampleRate, const std::string & filename) 
{
	this->_numGrains = numGrains;
	this->_sampleRate = sampleRate;
	this->_pointers.resize(this->_numGrains);
	this->_filePositions.resize(this->_numGrains);
	this->_grainSizes.resize(this->_numGrains);
	this->_muted.resize(this->_numGrains);
	
	// Set the starting values for each parameter
	if (!this->initializeAllParams(sampleRate)) {
		return false;
	}
	
	_sampleBuffer = AudioFileUtilities::loadMono(filename);
	if (_sampleBuffer.size() == 0) {
		printf("Failed loading file with path \"%s\"", filename.c_str());
		return false;
	}
	
	printf("Loaded sample with %d frames \n", (int)_sampleBuffer.size());

	//VIC
	this->_lastSample = _sampleBuffer.size()-1;

	return true;
}


float Granular::processGrain(int grainIdx)
{
	float out = 0.0;
	
	if (_muted[grainIdx]) {
		return out;
	}
	
	float pos = _pointers[grainIdx] + _filePositions[grainIdx];
	int prevFrame = floorf(pos);
	
	if (prevFrame >= this->_sampleBuffer.size()) {
		//VIC
		// this->_pointers[grainIdx] = 0;
		// return 0.0;
		// force last samples
		prevFrame = this->_sampleBuffer.size()-2;
		pos = 0.5;
	}

	
	float fracBelow = pos - prevFrame;
	out = interpolateCubic(this->_sampleBuffer[prevFrame-1], this->_sampleBuffer[prevFrame], this->_sampleBuffer[prevFrame+1], this->_sampleBuffer[prevFrame+2], fracBelow);
	
	// Calculate and apply amplitude windowing
	float windowVal =  applyWindow(this->_pointers[grainIdx], this->_grainSizes[grainIdx]);
	out = out * .75 * windowVal;
	
	this->_pointers[grainIdx] += this->_playbackSpeed;

	//	end of grain 
	if (this->_pointers[grainIdx] >= _grainSizes[grainIdx]) { //VIC added =
		this->_pointers[grainIdx] = 0;
		this->updateGrain(grainIdx);
	}

    return out;
}

float Granular::process() 
{
	//printf("___-this->_playbackSpeed %f\n", this->_playbackSpeed);
	float out = 0;
	
	for (int i = 0; i < this->_numGrains; i++) {
		out += this->processGrain(i);
	}

	return out;
}


void Granular::setGrainSizeMS(float grainSize)
{
	_grainSize = grainSize/1000 * _sampleRate;
}

void Granular::setFilePositionMS(float filePosition)
{
	_filePosition = filePosition/1000 * _sampleRate;
}

void Granular::setFilePosition(float filePosition)
{
	_filePosition = filePosition*_lastSample;
}

void Granular::setPlaybackSpeed(float playbackSpeed)
{
	_playbackSpeed = playbackSpeed;
}

void Granular::setProbability(float probability) 
{
	_probability = probability*100;
}

void Granular::setFilePositionRandomness(float randomness) {
	_filePositionRandomness = randomness;
}

void Granular::setGrainSizeRandomness(float randomness) {
	_grainSizeRandomness = randomness;
}


void Granular::updateAllGrains() {
	for (int i = 0; i > _numGrains; i++) {
		updateGrain(i);
	}
}











