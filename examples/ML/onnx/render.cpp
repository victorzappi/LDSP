#include "LDSP.h"
#include "OrtModel.h"
#include <iostream>
#include <cmath> // sin

float frequency = 440.0;
float amplitude = 0.2;

//------------------------------------------------
float phase;
float inverseSampleRate;

OrtModel model;


bool setup(LDSPcontext *context, void *userData) {


    /*
        An example of inference on a model with the following shape:

        (fc1): Linear(in_features=1, out_features=500, bias=True)
        (fc2): Linear(in_features=500, out_features=500, bias=True)
        (fc3): Linear(in_features=500, out_features=1, bias=True)
    */


    // Initialize ONNX Runtime Model Wrapper
    
    bool success = model.setup(
        "hello_world",                    // ORT environment name
        "./models/model.onnx"             // path to onnx model
    );


    if (!success) {
        std::cout << "failed to setup" << std::endl;
        return 1;
    }

    // // Setup Input + Output Tensors
    // float input[1][1][1] = {{{0.5}}};
    // float output[1];

    // // Run the model
    // model.run(input, output);

    // printf("input: %f\n", input[0][0][0]);
    // printf("output: %f\n",output[0]);


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

        // Run the model
        model.run(input, output);

		float out = amplitude * sinf(phase);
		phase += 2.0f * (float)M_PI * frequency * inverseSampleRate;
		while(phase > 2.0f *M_PI)
			phase -= 2.0f * (float)M_PI;
		
		for(int chn=0; chn<context->audioOutChannels; chn++)
            audioWrite(context, n, chn, out);
	}
}

void cleanup(LDSPcontext *context, void *userData) {}