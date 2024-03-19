#include "LDSP.h"
#include "OrtModel.h"


std::string modelType = "onnx";
std::string modelName = "AutoGuitarAmp";
int numInputSamples = 1;

OrtModel ortModel;

float input[1] = {0};
float output[1];


bool setup(LDSPcontext *context, void *userData) {

  std::string modelPath = "./models/"+modelName+"."+modelType;
  if (!ortModel.setup("session1", modelPath.c_str())) {
    printf("unable to setup ortModel");
  }

  return true;
}

void render(LDSPcontext *context, void *userData) 
{
   for(int n=0; n<context->audioFrames; n++) {

    input[0] = audioRead(context, n, 0);

    // Run the model
    ortModel.run(input, output);

    // passthrough test, because the model may not be trained
    audioWrite(context, n, 0, input[0]);
    audioWrite(context, n, 1, input[0]);
  }
}

void cleanup(LDSPcontext *context, void *userData)
{
}

