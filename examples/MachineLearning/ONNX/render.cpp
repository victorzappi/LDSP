#include "LDSP.h"
#include <libraries/OrtModel/OrtModel.h>
#include <cmath> // sin

float frequency = 440.0;
float amplitude = 0.1;

std::string modelType = "onnx";
std::string modelName = "model";
int numInputSamples = 1;

//------------------------------------------------
float phase;
float inverseSampleRate;

OrtModel ortModel(true);

float input[1] = {0};
float output[1];


bool setup(LDSPcontext *context, void *userData) 
{
    // Initialize ONNX Runtime Model Wrapper
    std::string modelPath = "./"+modelName+"."+modelType;
    if (!ortModel.setup("session1", modelPath)) // ORT session name, path to onnx model
        printf("unable to setup ortModel");


    // for sine
    inverseSampleRate = 1.0 / context->audioSampleRate;
	phase = 0.0;

    return true;
}

void render(LDSPcontext *context, void *userData) 
{
    for(int n=0; n<context->audioFrames; n++) 
    {

        if( (n%32) == 0)
        {
            input[0] = 0.5; // any number
            
            // Run the model
            ortModel.run(input, output); // we ignore the output
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
