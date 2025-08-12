/*
 * [2-Clause BSD License]
 *
 * Copyright 2022 Victor Zappi
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * LDSP Gesture Classifier using RapidLib
 * Classifies phone gestures using accelerometer data
 * Controls pitch of sinusoid based on classification
 */

#include "LDSP.h"
#include "rapidLib.h"
#include <vector>
#include <cmath>
#include <iostream>

// User settings
float class0Frequency = 220.0f;  // Frequency for Class 0
float class1Frequency = 880.0f;  // Frequency for Class 1

float frequencyChangeSpeed = 0.3f;  // 0.01 = very slow, 0.5 = fast, 1.0 = instant
float amplitudeChangeSpeed = 0.05f;  // 0.01 = very slow, 0.5 = fast, 1.0 = instant (only to/from idle)

//------------------------------------------------

// RapidLib classifier
rapidLib::classification classifier;

// FSM States
enum State {
    IDLE,
    RECORDING_CLASS_0,
    RECORDING_CLASS_1,
    CLASSIFYING
};

State currentState = IDLE;

// Training data
std::vector<rapidLib::trainingExample> trainingSet;
std::vector<std::vector<double>> currentGesture;  // Temporal storage for current gesture being recorded
int class0Examples = 0;
int class1Examples = 0;

// Button states (we need edge detection)
bool volumeUpPressed = false;
bool volumeDownPressed = false;
bool powerPressed = false;
bool prevVolumeUp = false;
bool prevVolumeDown = false;
bool prevPower = false;

// Audio synthesis
float frequency = class0Frequency;
float targetFrequency = class0Frequency;
float amplitude = 0.0f;  // Start silent
float targetAmplitude = 0.0f;
float phase = 0.0f;
float inverseSampleRate;

// For real-time classification
std::vector<std::vector<double>> recentSamples;
int slidingWindowSize = 0;  // Will be calculated in setup()
int previousPredictedClass = -1;  // Track class changes



// Helper function to compute mean and standard deviation features from gesture data
std::vector<double> computeGestureFeatures(const std::vector<std::vector<double>>& gestureData) 
{
    double meanX = 0, meanY = 0, meanZ = 0;
    double stdX = 0, stdY = 0, stdZ = 0;
    
    // Calculate means
    for (const auto& sample : gestureData) 
	{
        meanX += sample[0];
        meanY += sample[1];
        meanZ += sample[2];
    }
    meanX /= gestureData.size();
    meanY /= gestureData.size();
    meanZ /= gestureData.size();
    
    // Calculate standard deviations
    for (const auto& sample : gestureData) 
	{
        stdX += pow(sample[0] - meanX, 2);
        stdY += pow(sample[1] - meanY, 2);
        stdZ += pow(sample[2] - meanZ, 2);
    }
    stdX = sqrt(stdX / gestureData.size());
    stdY = sqrt(stdY / gestureData.size());
    stdZ = sqrt(stdZ / gestureData.size());
    
    return {meanX, meanY, meanZ, stdX, stdY, stdZ};
}

// Helper function to handle gesture recording
void handleGestureRecording(State& nextState, int classLabel, const std::string& className,
                           std::vector<std::vector<double>>& currentGesture,
                           std::vector<rapidLib::trainingExample>& trainingSet,
                           int& exampleCount) 
{
    if (currentGesture.size() > 10) 
	{
        // Compute features from the gesture
        std::vector<double> features = computeGestureFeatures(currentGesture);
        
        // Create training example
        rapidLib::trainingExample example;
        example.input = features;
        example.output = {(double)classLabel};
        trainingSet.push_back(example);
        exampleCount++;
        
        // Update state and print info
        nextState = IDLE;
        std::cout << "<<< " << className << " gesture recorded! (Total: " << exampleCount << ")" << std::endl;
        std::cout << "Samples collected: " << currentGesture.size() << std::endl;
        std::cout << "Features: [" << features[0] << ", " << features[1] << ", " << features[2] 
                 << ", " << features[3] << ", " << features[4] << ", " << features[5] << "]" << std::endl;
        std::cout << "State: IDLE\n" << std::endl;
    }
}

bool setup(LDSPcontext *context, void *userData) 
{
    // Check if accelerometer is available
    if (!context->sensorsSupported[chn_sens_accelX] ||
        !context->sensorsSupported[chn_sens_accelY] ||
        !context->sensorsSupported[chn_sens_accelZ]) 
	{
        printf("Accelerometer not present on this phone, the example cannot be run :(\n");
        return false;
    }
    
    inverseSampleRate = 1.0f / context->audioSampleRate;
    phase = 0.0f;
    currentState = IDLE;
    previousPredictedClass = -1;
    
    // Calculate sliding window size for ~500ms of data
    float bufferPeriod = (float)context->audioFrames / context->audioSampleRate;  // Time per buffer in seconds
    slidingWindowSize = (int)(0.5f / bufferPeriod);  // Number of buffers for 500ms
    
    std::cout << "\n=== LDSP Gesture Classifier ===" << std::endl;
    std::cout << "Controls:" << std::endl;
    std::cout << "  Volume UP: Start/stop recording gesture for Class 0 (neutral pose)" << std::endl;
    std::cout << "  Volume DOWN: Start/stop recording gesture for Class 1" << std::endl;
    std::cout << "  Power: Train classifier (if examples exist) or reset" << std::endl;
    std::cout << "\nState: IDLE (silent)\n" << std::endl;
    
    return true;
}

void render(LDSPcontext *context, void *userData) 
{
    // Read accelerometer data
    float accelX = sensorRead(context, chn_sens_accelX);
    float accelY = sensorRead(context, chn_sens_accelY);
    float accelZ = sensorRead(context, chn_sens_accelZ);
    
    // Read button states directly
    volumeUpPressed = buttonRead(context, chn_btn_volUp);
    volumeDownPressed = buttonRead(context, chn_btn_volDown);
    powerPressed = buttonRead(context, chn_btn_power);

    // Detect button presses (edge detection)
    bool volumeUpTrigger = volumeUpPressed && !prevVolumeUp;
    bool volumeDownTrigger = volumeDownPressed && !prevVolumeDown;
    bool powerTrigger = powerPressed && !prevPower;
    
    // Update button history
    prevVolumeUp = volumeUpPressed;
    prevVolumeDown = volumeDownPressed;
    prevPower = powerPressed;
    
    // FSM Logic
    switch(currentState) 
	{
        case IDLE:
            targetAmplitude = 0.0f;  // Silent
            
            if(volumeUpTrigger) 
			{
                currentState = RECORDING_CLASS_0;
                currentGesture.clear();
                std::cout << ">>> Recording Class 0 gesture... Keep the phone in neutral pose!" << std::endl;
            } 
			else if(volumeDownTrigger) 
			{
                currentState = RECORDING_CLASS_1;
                currentGesture.clear();
                std::cout << ">>> Recording Class 1 gesture... Move the phone!" << std::endl;
            } 
			else if(powerTrigger) 
			{
                // Either train (if we have examples of both classes) or reset (if any examples exist)
                if(class0Examples > 0 && class1Examples > 0) 
				{
                    // Train if we have examples of both classes
                    bool success = classifier.train(trainingSet);
                    if(success) 
					{
                        currentState = CLASSIFYING;
                        targetAmplitude = 0.2f;
                        recentSamples.clear();
                        std::cout << "*** Classifier trained successfully! ***" << std::endl;
                        std::cout << "Class 0 examples: " << class0Examples << std::endl;
                        std::cout << "Class 1 examples: " << class1Examples << std::endl;
                        std::cout << "State: CLASSIFYING (making sound)\n" << std::endl;
                    } 
					else
                        std::cout << "!!! Training failed !!!" << std::endl;
                } 
				else if(class0Examples > 0 || class1Examples > 0) 
				{
                    // Reset if we have any examples but not both classes
                    trainingSet.clear();
                    class0Examples = 0;
                    class1Examples = 0;
                    std::cout << "\n*** RESET - Incomplete training data deleted ***" << std::endl;
                    std::cout << "To train, provide examples from both classes" << std::endl;
                    std::cout << "State: IDLE (silent)\n" << std::endl;
                }
            }
            break;
            
        case RECORDING_CLASS_0:
            // Collect accelerometer data every audio buffer
            {
                std::vector<double> sample = {(double)accelX, (double)accelY, (double)accelZ};
                currentGesture.push_back(sample);
            }
            
            // Stop recording on button press
            if(volumeUpTrigger)
                handleGestureRecording(currentState, 0, "Class 0", currentGesture, trainingSet, class0Examples);
            break;
            
        case RECORDING_CLASS_1:
            // Collect accelerometer data every audio buffer
            {
                std::vector<double> sample = {(double)accelX, (double)accelY, (double)accelZ};
                currentGesture.push_back(sample);
            }
            
            // Stop recording on button press
            if(volumeDownTrigger)
                handleGestureRecording(currentState, 1, "Class 1", currentGesture, trainingSet, class1Examples);
            break;
            
        case CLASSIFYING:
            targetAmplitude = 0.2f;  // Make sound
            
            // Continuously classify current accelerometer data
            // Collect samples for sliding window
            recentSamples.push_back({(double)accelX, (double)accelY, (double)accelZ});
            
            // Keep a sliding window of ~500ms based on actual sample rate and buffer size
            if(recentSamples.size() > (size_t)slidingWindowSize)
                recentSamples.erase(recentSamples.begin());
            
            // Classify when we have enough samples (at least ~130ms of data)
            if(recentSamples.size() >= (size_t)(slidingWindowSize / 4)) 
			{
                // Calculate features from sliding window
                std::vector<double> features = computeGestureFeatures(recentSamples);
                
                // Classify
                std::vector<double> output = classifier.run(features);
                int predictedClass = (output.size() > 0 && output[0] > 0.5) ? 1 : 0;
                
                // Set frequency based on class
                targetFrequency = (predictedClass == 0) ? class0Frequency : class1Frequency;
                
                // Print only when class changes
                if(predictedClass != previousPredictedClass) 
				{
                    std::cout << "*** Class changed: " << previousPredictedClass << " -> " << predictedClass 
                             << " | Freq: " << targetFrequency << " Hz" << std::endl;
                    previousPredictedClass = predictedClass;
                }
            }
            
            // Reset on power button
            if(powerTrigger) 
			{
                trainingSet.clear();
                class0Examples = 0;
                class1Examples = 0;
                currentState = IDLE;
                targetAmplitude = 0.0f;
                recentSamples.clear();
                previousPredictedClass = -1;  // Reset class tracking
                std::cout << "\n*** RESET - All training data deleted ***" << std::endl;
                std::cout << "State: IDLE (silent)\n" << std::endl;
            }
            break;
    }
    
    // Smooth parameter changes (faster response for frequency)
    amplitude = amplitude * (1.0f - amplitudeChangeSpeed) + targetAmplitude * amplitudeChangeSpeed;
	frequency = frequency * (1.0f - frequencyChangeSpeed) + targetFrequency * frequencyChangeSpeed;
    
    // Generate audio output
    for(int n = 0; n < context->audioFrames; n++)
	{
        float out = amplitude * sinf(phase);
        phase += 2.0f * (float)M_PI * frequency * inverseSampleRate;
        
        while(phase > 2.0f * M_PI)
            phase -= 2.0f * M_PI;
        
        // Write to all output channels
        for(int chn = 0; chn < context->audioOutChannels; chn++)
            audioWrite(context, n, chn, out);
    }
}

void cleanup(LDSPcontext *context, void *userData) 
{
    std::cout << "LDSP Gesture Classifier Cleanup" << std::endl;
    trainingSet.clear();
    currentGesture.clear();
    recentSamples.clear();
}