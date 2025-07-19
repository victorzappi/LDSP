#include "Oscillator.h"
#include <cmath>


void Oscillator::setup(float fs, Oscillator::Type type)
{
	invSampleRate_ = 1.0 / fs;
	setType(type);
	phase_ = 0;
}

float Oscillator::process(float frequency) {
	setFrequency(frequency);
	return process();
}

float Oscillator::process() {
	float out;
	switch(type_) {
		default:
		case sine:
			out = sin(phase_);
			break;
		case triangle:
			if (phase_ > 0) {
			      out = -1 + (2 * phase_ / (float)M_PI);
			} else {
			      out = -1 - (2 * phase_/ (float)M_PI);
			}
			break;
		case square:
			if (phase_ > 0) {
			      out = 1;
			} else {
			      out = -1;
			}
			break;
		case sawtooth:
			out = 1.f / (float)M_PI * phase_;
			break;
	}
	computePhase();
	return out;
}

void Oscillator::computePhase(){
	phase_ += 2.0f * (float)M_PI * frequency_ * invSampleRate_;
	if(phase_ > M_PI)
		phase_ -= 2.0f * (float)M_PI;
}
