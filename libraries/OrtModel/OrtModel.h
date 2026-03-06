#include "onnxruntime_cxx_api.h"
#include <string>

using std::string;

class OrtModel {
public:

    OrtModel() {}
    OrtModel(bool _verbose) :  verbose(_verbose) {}
    ~OrtModel() {}

    bool setup(string _sessionName, string _modelPath, bool _multiThreading=false);
    void cleanup();
    
    void run(float* input, float* output); // single input note
    void run(float* input, float* params, float* output); // single input node + cond params
    void run(float** inputs, float* output); // multiple input nodes
    void run(float** inputs, float** outputs); // multiple input/output nodes

    // ── Accessors for buffer allocation ──────────────────────────────────────
    size_t getNumInputs()  const { return numInputNodes; }
    size_t getNumOutputs() const { return numOutputNodes; }
    size_t getInputSize(int i)  const { return inputTensorSizes[i]; }
    size_t getOutputSize(int i) const { return outputTensorSizes[i]; }

private: 
    bool verbose = true;

    // Holds onnx runtime session object, everything needed to interface with model
    Ort::Env * env = nullptr;
    Ort::Session * session = nullptr;

    // Path to ONNX Model file
    const char * modelPath = nullptr;
    // Name of the current onnx API session (helpful when having multiple model instances)
    const char * sessionName = nullptr;

    // Number of inputs to the model
    size_t numInputNodes = 0;
    // Names of each of the inputs to the model
    std::vector<const char*> inputNodeNames;
    // Tensor Dimensions of each input
    std::vector<std::vector<int64_t>> inputNodeDims;
    // Aggregated tensor size for each input
    std::vector<size_t> inputTensorSizes;

    // Number of outputs to the model
    size_t numOutputNodes = 0;
    // Names of each of the outputs to the model
    std::vector<const char*> outputNodeNames;
    // Tensor Dimensions of each output 
    std::vector<std::vector<int64_t>> outputNodeDims;
    // Aggregated tensor size for each output
    std::vector<size_t> outputTensorSizes;

    // Tensors
    std::vector<std::vector<float>> inputTensorValues;
    std::vector<std::vector<float>> outputTensorValues;
    std::vector<Ort::Value> inputTensors;
    std::vector<Ort::Value> outputTensors;
    
};