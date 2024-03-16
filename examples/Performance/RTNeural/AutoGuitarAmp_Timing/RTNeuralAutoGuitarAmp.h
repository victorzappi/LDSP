#pragma once

#include <RTNeural/RTNeural.h>

class RT_AutoGuitarAmp
{
public:
    RT_AutoGuitarAmp() = default;

    void reset();
    void load_json(const char* filename);
    template <typename T1>

    void set_weights(T1 model, const char* filename);

    void process(float* inData, float* outData, int numSamples);
    void process(float* inData, float param, float* outData, int numSamples);
    void process(float* inData, float param1, float param2, float* outData, int numSamples);

    int input_size = 1;

private:
    RTNeural::ModelT<float, 1, 1,
        RTNeural::LSTMLayerT<float, 1, 20>,
        RTNeural::DenseT<float, 20, 1>> model;

    RTNeural::ModelT<float, 2, 1,
        RTNeural::LSTMLayerT<float, 2, 20>,
        RTNeural::DenseT<float, 20, 1>> model_cond1;

    RTNeural::ModelT<float, 3, 1,
        RTNeural::LSTMLayerT<float, 3, 20>,
        RTNeural::DenseT<float, 20, 1>> model_cond2;

    // Pre-Allowcate arrays for feeding the models
    float inArray1[2] = { 0.0, 0.0 };
    float inArray2[3] = { 0.0, 0.0, 0.0 };
};




inline void RT_AutoGuitarAmp::process(float* inData, float* outData, int numSamples)
{
    for (int i = 0; i < numSamples; ++i)
        outData[i] = model.forward(inData + i) + inData[i];
}

inline void RT_AutoGuitarAmp::process(float* inData, float param, float* outData, int numSamples)
{
    for (int i = 0; i < numSamples; ++i) 
    {
        inArray1[0] = inData[i];
        inArray1[1] = param;
        outData[i] = model_cond1.forward(inArray1) + inData[i];
    }
}

inline void RT_AutoGuitarAmp::process(float* inData, float param1, float param2, float* outData, int numSamples)
{
    for (int i = 0; i < numSamples; ++i) 
    {
        inArray2[0] = inData[i];
        inArray2[1] = param1;
        inArray2[2] = param2;
        outData[i] = model_cond2.forward(inArray2) + inData[i];
    }
}