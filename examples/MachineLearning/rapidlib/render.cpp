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
#include <rapidLib.h>
#include <vector>
#include <cmath>
#include <iostream>

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
float frequency = 440.0f;
float targetFrequency = 440.0f;
float amplitude = 0.0f;  // Start silent
float targetAmplitude = 0.0f;
float phase = 0.0f;
float inverseSampleRate;

// For real-time classification
std::vector<std::vector<double>> recentSamples;
int slidingWindowSize = 0;  // Will be calculated in setup()
int classificationPrintInterval = 0;  // Will be calculated in setup()

//------------------------------------------------

void detectButtonEdges(LDSPcontext *context) {
    // Read actual button states from LDSP
    volumeUpPressed = buttonRead(context, chn_btn_volUp);
    volumeDownPressed = buttonRead(context, chn_btn_volDown);
    powerPressed = buttonRead(context, chn_btn_power);
}

bool setup(LDSPcontext *context, void *userData) {
    // Check if accelerometer is available
    if (!context->sensorsSupported[chn_sens_accelX] ||
        !context->sensorsSupported[chn_sens_accelY] ||
        !context->sensorsSupported[chn_sens_accelZ]) {
        printf("Accelerometer not present on this phone, the example cannot be run :(\n");
        return false;
    }
    
    inverseSampleRate = 1.0f / context->audioSampleRate;
    phase = 0.0f;
    currentState = IDLE;
    
    // Calculate sliding window size for ~500ms of data
    float bufferPeriod = (float)context->audioFrames / context->audioSampleRate;  // Time per buffer in seconds
    slidingWindowSize = (int)(0.5f / bufferPeriod);  // Number of buffers for 500ms
    
    // Calculate print interval for ~1 second
    classificationPrintInterval = (int)(1.0f / bufferPeriod);
    
    std::cout << "\n=== LDSP Gesture Classifier ===" << std::endl;
    std::cout << "Controls:" << std::endl;
    std::cout << "  Volume UP: Start/stop recording gesture for Class 0" << std::endl;
    std::cout << "  Volume DOWN: Start/stop recording gesture for Class 1" << std::endl;
    std::cout << "  Power: Train classifier (if examples exist) or reset" << std::endl;
    std::cout << "\nState: IDLE (silent)\n" << std::endl;
    
    return true;
}

void render(LDSPcontext *context, void *userData) {
    // Read accelerometer data
    float accelX = sensorRead(context, chn_sens_accelX);
    float accelY = sensorRead(context, chn_sens_accelY);
    float accelZ = sensorRead(context, chn_sens_accelZ);
    
    // Detect button presses (edge detection)
    detectButtonEdges(context);
    bool volumeUpTrigger = volumeUpPressed && !prevVolumeUp;
    bool volumeDownTrigger = volumeDownPressed && !prevVolumeDown;
    bool powerTrigger = powerPressed && !prevPower;

    // Update button history
    prevVolumeUp = volumeUpPressed;
    prevVolumeDown = volumeDownPressed;
    prevPower = powerPressed;
    
    // FSM Logic
    switch(currentState) {
        case IDLE:
            targetAmplitude = 0.0f;  // Silent
            
            if (volumeUpTrigger) {
                currentState = RECORDING_CLASS_0;
                currentGesture.clear();
                std::cout << ">>> Recording Class 0 gesture... Move the phone!" << std::endl;
            } else if (volumeDownTrigger) {
                currentState = RECORDING_CLASS_1;
                currentGesture.clear();
                std::cout << ">>> Recording Class 1 gesture... Move the phone!" << std::endl;
            } else if (powerTrigger && class0Examples > 0 && class1Examples > 0) {
                // Train if we have examples
                bool success = classifier.train(trainingSet);
                if (success) {
                    currentState = CLASSIFYING;
                    targetAmplitude = 0.2f;
                    recentSamples.clear();
                    std::cout << "*** Classifier trained successfully! ***" << std::endl;
                    std::cout << "Class 0 examples: " << class0Examples << std::endl;
                    std::cout << "Class 1 examples: " << class1Examples << std::endl;
                    std::cout << "State: CLASSIFYING (making sound)\n" << std::endl;
                } else {
                    std::cout << "!!! Training failed !!!" << std::endl;
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
            if (volumeUpTrigger && currentGesture.size() > 10) {
                // Calculate features from the gesture (mean and std)
                double meanX = 0, meanY = 0, meanZ = 0;
                double stdX = 0, stdY = 0, stdZ = 0;
                
                for (auto& sample : currentGesture) {
                    meanX += sample[0];
                    meanY += sample[1];
                    meanZ += sample[2];
                }
                meanX /= currentGesture.size();
                meanY /= currentGesture.size();
                meanZ /= currentGesture.size();
                
                for (auto& sample : currentGesture) {
                    stdX += pow(sample[0] - meanX, 2);
                    stdY += pow(sample[1] - meanY, 2);
                    stdZ += pow(sample[2] - meanZ, 2);
                }
                stdX = sqrt(stdX / currentGesture.size());
                stdY = sqrt(stdY / currentGesture.size());
                stdZ = sqrt(stdZ / currentGesture.size());
                
                // Create training example with features
                std::vector<double> features = {meanX, meanY, meanZ, stdX, stdY, stdZ};
                rapidLib::trainingExample example;
                example.input = features;
                example.output = {0.0};  // Class 0
                trainingSet.push_back(example);
                class0Examples++;
                
                currentState = IDLE;
                std::cout << "<<< Class 0 gesture recorded! (Total: " << class0Examples << ")" << std::endl;
                std::cout << "Samples collected: " << currentGesture.size() << std::endl;
                std::cout << "Features: [" << meanX << ", " << meanY << ", " << meanZ 
                         << ", " << stdX << ", " << stdY << ", " << stdZ << "]" << std::endl;
                std::cout << "State: IDLE\n" << std::endl;
            }
            break;
            
        case RECORDING_CLASS_1:
            // Collect accelerometer data every audio buffer
            {
                std::vector<double> sample = {(double)accelX, (double)accelY, (double)accelZ};
                currentGesture.push_back(sample);
            }
            
            // Stop recording on button press
            if (volumeDownTrigger && currentGesture.size() > 10) {
                // Calculate features from the gesture
                double meanX = 0, meanY = 0, meanZ = 0;
                double stdX = 0, stdY = 0, stdZ = 0;
                
                for (auto& sample : currentGesture) {
                    meanX += sample[0];
                    meanY += sample[1];
                    meanZ += sample[2];
                }
                meanX /= currentGesture.size();
                meanY /= currentGesture.size();
                meanZ /= currentGesture.size();
                
                for (auto& sample : currentGesture) {
                    stdX += pow(sample[0] - meanX, 2);
                    stdY += pow(sample[1] - meanY, 2);
                    stdZ += pow(sample[2] - meanZ, 2);
                }
                stdX = sqrt(stdX / currentGesture.size());
                stdY = sqrt(stdY / currentGesture.size());
                stdZ = sqrt(stdZ / currentGesture.size());
                
                // Create training example with features
                std::vector<double> features = {meanX, meanY, meanZ, stdX, stdY, stdZ};
                rapidLib::trainingExample example;
                example.input = features;
                example.output = {1.0};  // Class 1
                trainingSet.push_back(example);
                class1Examples++;
                
                currentState = IDLE;
                std::cout << "<<< Class 1 gesture recorded! (Total: " << class1Examples << ")" << std::endl;
                std::cout << "Samples collected: " << currentGesture.size() << std::endl;
                std::cout << "Features: [" << meanX << ", " << meanY << ", " << meanZ 
                         << ", " << stdX << ", " << stdY << ", " << stdZ << "]" << std::endl;
                std::cout << "State: IDLE\n" << std::endl;
            }
            break;
            
        case CLASSIFYING:
            targetAmplitude = 0.2f;  // Make sound
            
            // Continuously classify current accelerometer data
            // Collect samples for sliding window
            recentSamples.push_back({(double)accelX, (double)accelY, (double)accelZ});
            
            // Keep a sliding window of ~500ms based on actual sample rate and buffer size
            if (recentSamples.size() > (size_t)slidingWindowSize) {
                recentSamples.erase(recentSamples.begin());
            }
            
            // Classify when we have enough samples (at least ~130ms of data)
            if (recentSamples.size() >= (size_t)(slidingWindowSize / 4)) {
                // Calculate features from sliding window
                double meanX = 0, meanY = 0, meanZ = 0;
                double stdX = 0, stdY = 0, stdZ = 0;
                
                for (auto& sample : recentSamples) {
                    meanX += sample[0];
                    meanY += sample[1];
                    meanZ += sample[2];
                }
                meanX /= recentSamples.size();
                meanY /= recentSamples.size();
                meanZ /= recentSamples.size();
                
                for (auto& sample : recentSamples) {
                    stdX += pow(sample[0] - meanX, 2);
                    stdY += pow(sample[1] - meanY, 2);
                    stdZ += pow(sample[2] - meanZ, 2);
                }
                stdX = sqrt(stdX / recentSamples.size());
                stdY = sqrt(stdY / recentSamples.size());
                stdZ = sqrt(stdZ / recentSamples.size());
                
                // Classify
                std::vector<double> features = {meanX, meanY, meanZ, stdX, stdY, stdZ};
                std::vector<double> output = classifier.run(features);
                int predictedClass = (output.size() > 0 && output[0] > 0.5) ? 1 : 0;
                
                // Set frequency based on class
                targetFrequency = (predictedClass == 0) ? 440.0f : 880.0f;
                
                // Print occasionally (every ~1 second)
                static int printCounter = 0;
                if (++printCounter >= classificationPrintInterval) {
                    printCounter = 0;
                    std::cout << "Class: " << predictedClass 
                             << " | Freq: " << targetFrequency << " Hz"
                             << " | Accel: [" << accelX << ", " << accelY << ", " << accelZ << "]" 
                             << std::endl;
                }
            }
            
            // Reset on power button
            if (powerTrigger) {
                trainingSet.clear();
                class0Examples = 0;
                class1Examples = 0;
                currentState = IDLE;
                targetAmplitude = 0.0f;
                recentSamples.clear();
                std::cout << "\n*** RESET - All training data deleted ***" << std::endl;
                std::cout << "State: IDLE (silent)\n" << std::endl;
            }
            break;
    }
    
    // Smooth parameter changes
    amplitude = amplitude * 0.95f + targetAmplitude * 0.05f;
    frequency = frequency * 0.98f + targetFrequency * 0.02f;
    
    // Generate audio output
    for(int n = 0; n < context->audioFrames; n++) {
        float out = amplitude * sinf(phase);
        phase += 2.0f * (float)M_PI * frequency * inverseSampleRate;
        
        while(phase > 2.0f * M_PI)
            phase -= 2.0f * M_PI;
        
        // Write to all output channels
        for(int chn = 0; chn < context->audioOutChannels; chn++) {
            audioWrite(context, n, chn, out);
        }
    }
}

void cleanup(LDSPcontext *context, void *userData) {
    std::cout << "LDSP Gesture Classifier Cleanup" << std::endl;
    trainingSet.clear();
    currentGesture.clear();
    recentSamples.clear();
}