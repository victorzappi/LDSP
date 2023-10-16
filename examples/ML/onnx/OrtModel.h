#include "onnxruntime_cxx_api.h"

class OrtModel {
public:

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
    std::vector<int64_t> inputNodeDims;

    // Number of outputs to the model
    size_t numOutputNodes;
    // Names of each of the outputs to the model
    std::vector<const char*> outputNodeNames;
    // Tensor Dimensions of each output 
    std::vector<int64_t> outputNodeDims;

    OrtModel() {}
    ~OrtModel() {}

    bool setup(const char * _sessionName, const char * _modelPath);
    void run();
    void run(float inputTensorData[1][1][1], float * outputTensorData);
    void cleanup();
    
};