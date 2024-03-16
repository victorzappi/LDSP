#include "LDSP.h"
#include <RTNeural/RTNeural.h>
#include "RTNeuralAutoGuitarAmp.h"
#include <chrono>
#include <fstream> // ofstream

RT_AutoGuitarAmp model;

float input[1];
float output[1] = {0};

int outputSize = 1;

//--------------------------------
std::string modelName = "AutoGuitarAmp";

unsigned long long * inferenceTimes;
int logPtr = 0;
constexpr int testDuration_sec = 10;
int numLogs;


bool setup(LDSPcontext *context, void *userData)
{
    model.reset();

    //--------------------------------
    inferenceTimes = new unsigned long long[context->audioSampleRate*testDuration_sec*1.01];
    numLogs = context->audioSampleRate*testDuration_sec / outputSize; // division to handle case of models outputting a block of samples

    return true;
}

void render(LDSPcontext *context, void *userData)
{
    for(int n=0; n<context->audioFrames; n++)
	{
        input[0] = audioRead(context, n, 0);
        float param = 0;

        // Start the Clock
        auto start_time = std::chrono::high_resolution_clock::now();
        
        model.process(input, param, output, 1);

        // Stop the clock  
        auto end_time = std::chrono::high_resolution_clock::now();
        inferenceTimes[logPtr] = std::chrono::duration_cast
                                <std::chrono::microseconds>(end_time - start_time).count();
        logPtr++;

        // passthrough test, because the model may not be trained
        audioWrite(context, n, 0, input[0]);
        audioWrite(context, n, 1, input[0]);

        if(logPtr>=numLogs)
            LDSP_requestStop();
	}
}

void cleanup(LDSPcontext *context, void *userData)
{
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
