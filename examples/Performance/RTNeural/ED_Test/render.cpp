#include "LDSP.h"
#include <RTNeural/RTNeural.h>
#include "RTNeuralED.h"

const int w = 16;
const int u = 64;
const int d = 4;

RT_ED<w, u, d> model;

float input[2*w] = {0};
float params[d] = {0};
float output[w] = {0};

int inputSize = 2*w;
int outputSize = w;
int inputCounter = 0;

const int circBuffLength = 48000; // a reasonably large buffer, to limit end-of-buffer overhead
int writePointer; 
int readPointer;
float circBuff[circBuffLength];


bool setup(LDSPcontext *context, void *userData)
{
    model.reset();

    writePointer = w; // the first w samples must be zeros
    readPointer = 0;

    if(context->audioFrames % w)
        printf("Warning! Period size (%d) is supposed to be an integer multiple of the output size w (%d)!\n", context->audioFrames, outputSize);

    return true;
}

void render(LDSPcontext *context, void *userData)
{
    for(int n=0; n<context->audioFrames; n++)
	{
        circBuff[writePointer] = audioRead(context,n,0);

        // run inference every w inputs
        if( ++inputCounter == outputSize )
        {
            inputCounter = 0;

            if(readPointer<=circBuffLength-inputSize)
                std::copy(circBuff + readPointer, circBuff + readPointer + inputSize, input);
            else 
            {
                int firstPartSize = circBuffLength - readPointer;
                std::copy(circBuff + readPointer, circBuff + circBuffLength, input);
                std::copy(circBuff, circBuff + (inputSize - firstPartSize), input + firstPartSize);
            }

            model.process(input, params, output); // outputs a block of w samples
            
            for(int out=0; out<outputSize; out++)
            {
                // passthrough test, because the model may not be trained
                audioWrite(context, n-outputSize+1+out, 0, input[outputSize+out]);
                audioWrite(context, n-outputSize+1+out, 1, input[outputSize+out]);
            }

            readPointer += outputSize;
            if( readPointer >= circBuffLength)
                readPointer -= circBuffLength;
        }
        
        if(++writePointer >= circBuffLength)
            writePointer = 0;
	}
}

void cleanup(LDSPcontext *context, void *userData)
{

}
