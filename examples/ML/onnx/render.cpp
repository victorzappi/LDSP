// benchmarking
#include <chrono>
#include "LDSP.h"
#include "OrtModel.h"
#include <iostream>
#include <cmath> // sin

float frequency = 440.0;
float amplitude = 0.1;

//------------------------------------------------
float phase;
float inverseSampleRate;

OrtModel model;


float benchmarkInference() {    
    // Measure the start time
    auto start_time = std::chrono::high_resolution_clock::now();

    float input[1][1][1] = {{{0.5}}};
    float output[1];
    // Run the model
    model.run(input, output);

    // Measure the end time
    auto end_time = std::chrono::high_resolution_clock::now();

    // Calculate the elapsed time
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);

    std::cout << "Model inference took: " << duration.count()/1000.0 << " milliseconds" << std::endl;

    return duration.count()/1000.0; 
}


bool setup(LDSPcontext *context, void *userData) {


    // Initialize ONNX Runtime Model Wrapper
    
    bool success = model.setup(
        "hello_world",                    // ORT environment name
        "./models/model.onnx"             // path to onnx model
    );


    if (!success) {
        std::cout << "failed to setup" << std::endl;
        return 1;
    }

    float inferenceSpeed = benchmarkInference();

    inverseSampleRate = 1.0 / context->audioSampleRate;
	phase = 0.0;


    return true;
}

void render(LDSPcontext *context, void *userData) 
{
    for(int n=0; n<context->audioFrames; n++)
	{

        // Setup Input + Output Tensors
        float input[1][1][1] = {{{0.5}}};
        float output[1];
        if (n%8==0) {
            // Run the model
            model.run(input, output);
        }


        // generate sine wave
		float out = amplitude * sinf(phase);
		phase += 2.0f * (float)M_PI * frequency * inverseSampleRate;
		while(phase > 2.0f *M_PI)
			phase -= 2.0f * (float)M_PI;
		
		for(int chn=0; chn<context->audioOutChannels; chn++)
            audioWrite(context, n, chn, out);
	}
}

void cleanup(LDSPcontext *context, void *userData) {}