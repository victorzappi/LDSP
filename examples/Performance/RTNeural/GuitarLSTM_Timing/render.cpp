#include "LDSP.h"
#include <RTNeural/RTNeural.h>
#include "RTNeuralGuitarLSTM.h"
#include <chrono>
#include <fstream> // ofstream

RT_GuitarLSTM model;

float *input;
float output[1] = {0};

int inputSize;
int outputSize = 1;

const int circBuffLength = 48000; // a reasonably large buffer, to limit end-of-buffer overhead
int writePointer;
int readPointer;
float circBuff[circBuffLength];

//--------------------------------
std::string modelName = "GuitarLSTM";

unsigned long long * inferenceTimes;
int logPtr = 0;
constexpr int testDuration_sec = 10;
int numLogs;


bool setup(LDSPcontext *context, void *userData)
{
    model.reset();

    inputSize = model.getInputSize();
    input = new float[inputSize];

    writePointer = inputSize-1; // the first intputSize-1 samples must be zeros
    readPointer = 0;

    //--------------------------------
    inferenceTimes = new unsigned long long[context->audioSampleRate*testDuration_sec*1.01];
    numLogs = context->audioSampleRate*testDuration_sec / outputSize; // division to handle case of models outputting a block of samples

    return true;
}

void render(LDSPcontext *context, void *userData)
{
    for(int n=0; n<context->audioFrames; n++)
	{
        circBuff[writePointer] = audioRead(context,n,0);

        if(readPointer<=circBuffLength-inputSize)
            std::copy(circBuff + readPointer, circBuff + readPointer + inputSize, input);
        else 
        {
            int firstPartSize = circBuffLength - readPointer;
            std::copy(circBuff + readPointer, circBuff + circBuffLength, input);
            std::copy(circBuff, circBuff + (inputSize - firstPartSize), input + firstPartSize);
        }

        // Start the Clock
        auto start_time = std::chrono::high_resolution_clock::now();

        model.process(input, output, 1);

        // Stop the clock  
        auto end_time = std::chrono::high_resolution_clock::now();
        inferenceTimes[logPtr] = std::chrono::duration_cast
                                <std::chrono::microseconds>(end_time - start_time).count();
        logPtr++;

        // passthrough test, because the model may not be trained
        audioWrite(context, n, 0, input[inputSize-1]);
        audioWrite(context, n, 1, input[inputSize-1]);

        if(++readPointer >= circBuffLength)
            readPointer = 0;
        if(++writePointer >= circBuffLength)
            writePointer = 0;	

        if(logPtr>=numLogs)
            LDSP_requestStop();
    }
}

void cleanup(LDSPcontext *context, void *userData)
{
    delete[] input;

    //--------------------------------
    std::string timingLogDir = ".";
    std::string timingLogFileName = "inferenceTiming_"+modelName+"_out"+std::to_string(outputSize)+"_rtneural.txt";
    std::string timingLogFilePath = timingLogDir+"/"+timingLogFileName;

    std::ofstream logFile(timingLogFilePath);
    if(logFile.is_open())
    {
        for (int i=0;i<numLogs; i++)
            logFile << std::to_string(inferenceTimes[i]) << "\n";
    }
    logFile.close();

    delete[] inferenceTimes;
}
