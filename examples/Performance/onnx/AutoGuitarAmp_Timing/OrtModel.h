#include "onnxruntime_cxx_api.h"

class OrtModel {
public:

    OrtModel() {}
    ~OrtModel() {}

    bool setup(const char * _sessionName, const char * _modelPath, bool _multiThreading = false);
    void cleanup();
    void run(float ** inputs, float * output);
    void run(float * input, float * output);
    void run(float * input, float * params, float * output);

private: 

    // Holds onnx runtime session object, everything needed to interface with model
    Ort::Env * env;
    Ort::Session * session;

    // Path to ONNX Model file
    const char * modelPath;
    // Name of the current onnx API session (helpful when having multiple model instances)
    const char * sessionName;

    // Number of inputs to the model
    size_t numInputNodes;
    // Names of each of the inputs to the model
    std::vector<const char*> inputNodeNames;
    // Tensor Dimensions of each input
    std::vector<std::vector<int64_t>> inputNodeDims;
    // Aggregated tensor size for each input
    std::vector<size_t> inputTensorSizes;

    // Number of outputs to the model
    size_t numOutputNodes;
    // Names of each of the outputs to the model
    std::vector<const char*> outputNodeNames;
    // Tensor Dimensions of each output 
    std::vector<std::vector<int64_t>> outputNodeDims;
    // Aggregated tensor size for each output
    std::vector<size_t> outputTensorSizes;


    std::vector<std::vector<float>> inputTensorValues;
    std::vector<std::vector<float>> outputTensorValues;
    std::vector<Ort::Value> inputTensors;
    std::vector<Ort::Value> outputTensors;
    
};