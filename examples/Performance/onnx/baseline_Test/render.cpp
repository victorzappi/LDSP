#include "LDSP.h"
#include "OrtModel.h"


std::string modelType = "onnx";
std::string modelName = "baseline";
int numInputSamples = 1;

OrtModel ortModel;

float input[1] = {0};
float output[1];


bool setup(LDSPcontext *context, void *userData) {

  std::string modelPath = "./models/"+modelName+"."+modelType;
  if (!ortModel.setup("session1", modelPath.c_str())) {
    printf("unable to setup ortModel\n");
  }

  return true;
}

void render(LDSPcontext *context, void *userData) 
{
  for(int n=0; n<context->audioFrames; n++) {

    float out = audioRead(context, n, 0);

    // Run the model
    ortModel.run(input, output);

    // passthrough test, because the model may not be trained
    audioWrite(context, n, 0, out);
    audioWrite(context, n, 1, out);
  }
}

void cleanup(LDSPcontext *context, void *userData)
{
}

