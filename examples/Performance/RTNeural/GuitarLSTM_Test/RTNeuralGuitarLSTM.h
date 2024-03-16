#pragma once

#include <RTNeural/RTNeural.h>

class RT_GuitarLSTM
{

#define INPUT_SIZE 5

public:
    RT_GuitarLSTM() = default;

    void reset();
    void load_json(const char* filename);
    
    template <typename T1>
    void set_weights(T1 model, const char* filename);

    int getInputSize();

    void process(float* inData, float* outData, int numSamples);

private:
    int input_size = INPUT_SIZE;
    RTNeural::ModelT<float, INPUT_SIZE, 1,
        RTNeural::Conv1DT<float, /*in_sizet=*/INPUT_SIZE, /*out_sizet(filter num)=*/16, /*kernel_size=*/12, /*dilation_rate =*/1>,
        RTNeural::Conv1DT<float, /*in_sizet=*/16, /*out_sizet(filter num)=*/16, /*kernel_size=*/12, /*dilation_rate =*/1>,
        RTNeural::LSTMLayerT<float, 16, 32>, // or 36
        RTNeural::DenseT<float, 32, 1>> model; // or 36

    // Pre-Allocate arrays for feeding the model
    alignas(32) float inArray[INPUT_SIZE] = {0.0};
};


inline void RT_GuitarLSTM::process(float* inData, float* outData, int numSamples)
{
    // here we expect taht inData is byte aligned and we have input_size-1 extra samples in input
    // i.e., inData contains numSamples + input_size-1 samples
    for (int i = 0; i < numSamples; ++i) 
    {
        std::copy(inData+i, inData+i + input_size, inArray);
        outData[i] = model.forward(inArray);
    }
}
