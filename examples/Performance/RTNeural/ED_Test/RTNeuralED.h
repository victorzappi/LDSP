#pragma once

#include <RTNeural/RTNeural.h>
#include "lstm_eigen_custom.h"

/* namespace RTNEURAL_NAMESPACE
{


template<typename T, int in_sizet, int out_sizet,
         SampleRateCorrectionMode sampleRateCorr = SampleRateCorrectionMode::None,
         typename MathsProvider = DefaultMathsProvider>
class CustomLSTMLayerT : public LSTMLayerT<T, in_sizet, out_sizet, sampleRateCorr, MathsProvider>
{
public:
    // Inherit constructors from LSTMLayerT
    using LSTMLayerT<T, in_sizet, out_sizet, sampleRateCorr, MathsProvider>::LSTMLayerT;

    // Method to explicitly set the LSTM layer's hidden and cell states
    void setStates(const T* hiddenState, const T* cellState)
    {
        std::copy(hiddenState, hiddenState + out_sizet, this->ht1);
        std::copy(cellState, cellState + out_sizet, this->ct1);
    }
};

} // namespace RTNEURAL_NAMESPACE */


template <std::size_t w, std::size_t u, std::size_t d>
class RT_ED
{
public:
    RT_ED();

    void reset();
    void load_json(const char* filename);
    void set_weights(const char* filename);

    void process(float* inData, float* condParams, float* outData);

private:
    // Encoder dense layers for conditional input data 
    RTNeural::ModelT<float, d, u,
        RTNeural::DenseT<float, d, u>> encoder_dense_h; // Dense layer for state_h
    RTNeural::ModelT<float, d, u,
        RTNeural::DenseT<float, d, u>> encoder_dense_c; // Dense layer for state_c

    // Encoder convolutional layers for past input data
    RTNeural::ModelT<float, w, u,
        RTNeural::Conv1DT<float, /*in_sizet=*/w, /*out_sizet(filter num)=*/u, /*kernel_size=*/w, /*dilation_rate =*/1>> encoder_conv_h; // Convolutional layer for state_h
    RTNeural::ModelT<float, w, u,
        RTNeural::Conv1DT<float, /*in_sizet=*/w, /*out_sizet(filter num)=*/u, /*kernel_size=*/w, /*dilation_rate =*/1>> encoder_conv_c; // Convolutional layer for state_c

    // Decoder which will receive the concatenated states from the encoder
    RTNeural::ModelT<float, w, w,
        RTNeural::CustomLSTMLayerT<float, w, u>, // LSTM layer
        RTNeural::DenseT<float, u, u>, // Additional Dense layer before activation, if needed
        RTNeural::SigmoidActivationT<float, u>, // Activation layer
        RTNeural::DenseT<float, u, w>> decoder; // Final dense output layer
    

    alignas(16) float condParams[d];
    alignas(16) float inDataBlock[2*w];
    alignas(16) float states_h[u];
    alignas(16) float states_c[u];
    Eigen::Matrix<float, u, 1> eigen_states_h;
    Eigen::Matrix<float, u, 1> eigen_states_c;
};



template <std::size_t w, std::size_t u, std::size_t d>
RT_ED<w, u, d>::RT_ED()
{
    eigen_states_h = Eigen::MatrixXf::Zero(u, 1);
    eigen_states_c = Eigen::MatrixXf::Zero(u, 1);
}

template <std::size_t w, std::size_t u, std::size_t d>
void RT_ED<w, u, d>::reset()
{
   encoder_dense_h.reset();
   encoder_dense_c.reset();

   encoder_conv_h.reset();
   encoder_conv_c.reset();

   decoder.reset();
}

template <std::size_t w, std::size_t u, std::size_t d>
void RT_ED<w, u, d>::load_json(const char* filename)
{
    std::ifstream file(filename);
    if (!file.is_open())
        throw std::runtime_error("Could not open the weights file.");

    // set_weights(weights_json);
}

template <std::size_t w, std::size_t u, std::size_t d>
void RT_ED<w, u, d>::set_weights(const char* filename)
{
    // Set weights and biases for each layer
}

template <std::size_t w, std::size_t u, std::size_t d>
inline void RT_ED<w, u, d>::process(float* inData, float* params, float* outData)
{    
    for(int i=0; i<d; i++)
        condParams[i] = params[i];

    // Process condParams through encoder dense layers
    encoder_dense_h.forward(condParams);
    const float *cond_dense_h = static_cast<const float*>( encoder_dense_h.getOutputs() );
    encoder_dense_c.forward(condParams);
    const float *cond_dense_c = static_cast<const float*>( encoder_dense_c.getOutputs() );


    std::copy(inData, inData + 2*w, inDataBlock);

    // for(int i = 0; i < numSamples; ++i)
    // {
        // Process block of least recent w samples through encoder convolutional layers
        encoder_conv_h.forward(/* inDataBlock+i */ inDataBlock);
        const float * conv_h = static_cast<const float*>( encoder_conv_h.getOutputs() );
        encoder_conv_c.forward(/* inDataBlock+i */ inDataBlock);
        const float * conv_c = static_cast<const float*>( encoder_conv_h.getOutputs() );

      
        // Prepare LSTM states input by combining convolutional and dense outputs
        for (size_t s = 0; s < u; ++s) 
        {
            states_h[s] = conv_h[s] + cond_dense_h[s];
            states_c[s] = conv_c[s] + cond_dense_c[s]; 
        }


        // Set the states in the decoder LSTM
        auto& lstmLayer = static_cast<RTNeural::CustomLSTMLayerT<float, w, u>&>(decoder.template get<0>());
        std::copy(states_h, states_h + u, eigen_states_h.data());
        std::copy(states_c, states_c + u, eigen_states_c.data());
        lstmLayer.setInitialHiddenState(eigen_states_h);
        lstmLayer.setInitialCellState(eigen_states_c);
    

        // Process block of most recent w samples through the decoder LSTM and dense layer
        //outData[i] = decoder.forward(inDataBlock + w + i);
        decoder.forward(inDataBlock + w);
        const float* output = static_cast<const float*>( decoder.getOutputs() );
        std::copy(output, output + w, outData);
    

    // }
 }

