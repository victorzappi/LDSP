#include "LDSP.h"
#include <RTNeural/RTNeural.h>
#include "RTNeuralLSTM.h"

RT_LSTM model;


bool setup(LDSPcontext *context, void *userData)
{
    /**
     * Model sourced from
     * https://github.com/GuitarML/NeuralPi
     */
    model.load_json("TS9_FullD.json");
    model.reset();

    return true;
}

void render(LDSPcontext *context, void *userData)
{
    for(int n=0; n<context->audioFrames; n++)
	{
        float original = audioRead(context, n, 0);

        const float input[] = { original };
        float output[] = { 0 };
        model.process(input, output, 1);

        audioWrite(context, n, 0, output[0]);
        audioWrite(context, n, 1, output[0]);
	}
}

void cleanup(LDSPcontext *context, void *userData)
{

}
