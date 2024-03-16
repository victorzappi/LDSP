#include "LDSP.h"
#include <RTNeural/RTNeural.h>
#include "RTNeuralAutoGuitarAmp.h"

RT_AutoGuitarAmp model;

float input[1];
float output[1] = {0};

bool setup(LDSPcontext *context, void *userData)
{
    model.reset();

    return true;
}

void render(LDSPcontext *context, void *userData)
{
    for(int n=0; n<context->audioFrames; n++)
	{
        input[0] = audioRead(context, n, 0);
        float param = 0;

        model.process(input, param, output, 1);
    
        // passthrough test, because the model may not be trained
        audioWrite(context, n, 0, input[0]);
        audioWrite(context, n, 1, input[0]);
	}
}

void cleanup(LDSPcontext *context, void *userData)
{

}
