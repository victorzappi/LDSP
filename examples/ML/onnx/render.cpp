#include "LDSP.h"
#include "OrtModel.h"
#include <iostream>


bool setup(LDSPcontext *context, void *userData) {


    /*
        An example of inference on a model with the following shape:

        (fc1): Linear(in_features=1, out_features=500, bias=True)
        (fc2): Linear(in_features=500, out_features=500, bias=True)
        (fc3): Linear(in_features=500, out_features=1, bias=True)
    */


    // Initialize ONNX Runtime Model Wrapper
    OrtModel model;
    bool success = model.setup(
        "hello_world",                     // ORT environment
        "../models/model.onnx"             // path to onnx model
    );


    if (!success) {
        std::cout << "failed to setup" << std::endl;
        return 1;
    }

    float input[] = {0.5};
    float output[1];

    model.run(input, output);

    printf("input: %f\n", input[0]);
    printf("output: %f\n",output[0]);

    return true;
}

void render(LDSPcontext *context, void *userData) {}

void cleanup(LDSPcontext *context, void *userData) {}