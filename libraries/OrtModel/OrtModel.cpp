#include "OrtModel.h"
#include <iostream>
#include <thread>

#include <numeric> // std::accumulate()

Ort::MemoryInfo memoryInfo = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
Ort::RunOptions options;

// this is taken from Domenico Stefani's OnnxTemplatePlugin
// https://github.com/domenicostefani/ONNXruntime-VSTplugin-template.git
template <typename T>
T vectorProduct(const std::vector<T> &v) 
{
  return std::accumulate(v.begin(), v.end(), 1, std::multiplies<T>());
}



bool OrtModel::setup(string _sessionName, string _modelPath, bool _multiThreading) 
{
    this->modelPath = _modelPath.c_str();
    this->sessionName = _sessionName.c_str();

    // Prepare session
    Ort::SessionOptions sessionOptions;
    Ort::AllocatorWithDefaultOptions allocator;
    sessionOptions.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
    sessionOptions.EnableCpuMemArena();
    if(_multiThreading) 
    {
        unsigned int max_threads = std::thread::hardware_concurrency();
        sessionOptions.SetIntraOpNumThreads(max_threads);
        if(verbose)
            printf("Session %s, max thread: %d\n",  _sessionName.c_str(), max_threads);
    }

    // Load Model
    try {
        this->env = new Ort::Env(OrtLoggingLevel::ORT_LOGGING_LEVEL_WARNING, _sessionName.c_str());
        this->session = new Ort::Session(*env, _modelPath.c_str(), sessionOptions);
        if(verbose)
            printf("\nLoaded Model: %s\n", _modelPath.c_str());
    } catch (const Ort::Exception& exception) {
        printf("Error loading model: %s\n", exception.what());
        return false;
    }

    // Get number of inputs/outputs to the model
    numInputNodes = this->session->GetInputCount();
    numOutputNodes = this->session->GetOutputCount();    

    if(verbose) 
    {
        printf("Model Dimensions:\n");
        printf("---Input: %zu\n", numInputNodes);
        printf("---Output: %zu\n", numOutputNodes);
    }

    // allocate space to hold names of model inputs
    inputNodeNames.resize(numInputNodes);
    inputNodeDims.resize(numInputNodes);
    // Gather metadata information about the model inputs
    for(int i = 0; i < numInputNodes; i++) {
    
        // Get names of each input node
        Ort::AllocatedStringPtr inputName = session->GetInputNameAllocated(i, allocator);
        if(verbose)
        {
            printf("==================================\n");
            printf("Input %d : name=%s\n", i, inputName.get());
        }
        inputNodeNames[i] = inputName.get();
        inputName.release();

        // Get Data type of each input node
        Ort::TypeInfo type_info = session->GetInputTypeInfo(i);
        auto tensorInfo = type_info.GetTensorTypeAndShapeInfo();

        ONNXTensorElementDataType type = tensorInfo.GetElementType();
        if(verbose)
            printf("Input %d : type=%d\n", i, type);

        // Get shapes of input tensors
        inputNodeDims[i] = tensorInfo.GetShape();
        if(verbose)
            printf("Input %d : num_dims=%zu\n", i, inputNodeDims[i].size());
        for(size_t j = 0; j < inputNodeDims[i].size(); j++) 
        {
            if(verbose)
                printf("Input %d : dim %d=%d\n", i, (int)j, (int)inputNodeDims[i][j]);
            if(inputNodeDims[i][j] <= 0) 
            {
                if(verbose)
                    printf("Warning: Input %d, dim %d has non-positive size, adjusting to size of 1\n", i, (int)j);
                inputNodeDims[i][j] = 1; // Adjust non-positive dimensions to 1
            }
        }

        inputTensorSizes.push_back(vectorProduct(inputNodeDims[i]));
        inputTensorValues.push_back(std::vector<float>(inputTensorSizes[i]));
    }


    // allocate space to hold names of model outputs
    outputNodeNames.resize(numOutputNodes);
    outputNodeDims.resize(numOutputNodes);
    // Gather metadata information about the model outputs
    for (int i = 0; i < numOutputNodes; i++) 
    { 
        // Get names of each output node
        Ort::AllocatedStringPtr outputName = session->GetOutputNameAllocated(i, allocator);
        if(verbose)
        {
            printf("==================================\n");
            printf("Output %d : name=%s\n", i, outputName.get());
        }
        outputNodeNames[i] = outputName.get();
        outputName.release();

        // Get Data type of each output node
        Ort::TypeInfo typeInfo = session->GetOutputTypeInfo(i);
        auto tensorInfo = typeInfo.GetTensorTypeAndShapeInfo();

        ONNXTensorElementDataType type = tensorInfo.GetElementType();
        if(verbose)
            printf("Output %d : type=%d\n", i, type);

        // Get shapes of output tensors
        outputNodeDims[i] = tensorInfo.GetShape();
        if(verbose)
            printf("Output %d : num_dims=%zu\n", i, outputNodeDims.size());
        for (int j = 0; j < outputNodeDims.size(); j++) {
            if(verbose)
                printf("Output %d : dim %d=%d\n", i, j, (int)outputNodeDims[i][j]);
            if ((int) this->outputNodeDims[i][j] < 0) {
                if(verbose)
                    printf("Output %d, dim %d has a variable size, forcing a size of 1", i,j);
                outputNodeDims[i][j] = outputNodeDims[i][j]*-1;
            }
        }

        outputTensorSizes.push_back(vectorProduct(outputNodeDims[i]));
        outputTensorValues.push_back(std::vector<float>(outputTensorSizes[i]));
    }
    if(verbose)
        printf("\n");

    // Prepare in/out tensors
    for (int i = 0; i < numInputNodes; i++) 
    {
        inputTensors.push_back(Ort::Value::CreateTensor<float>(
                                memoryInfo,
                                inputTensorValues[i].data(),
                                inputTensorValues[i].size(),
                                inputNodeDims[i].data(),
                                inputNodeDims[i].size()));
    }
    for (int i = 0; i < numOutputNodes; i++) 
    {
        outputTensors.push_back(Ort::Value::CreateTensor<float>(
            memoryInfo,
            outputTensorValues[i].data(),
            outputTensorValues[i].size(),
            outputNodeDims[i].data(),
            outputNodeDims[i].size()));
    }

    return true;   
}

// multiple input nodes
void OrtModel::run(float** inputs, float* output) 
{
    // Copy Inputs
    for(int i = 0; i < numInputNodes; i++) 
    {
        for (int j = 0; j < inputTensorSizes[i]; j++)
            inputTensorValues[i][j] = inputs[i][j];
    }

    // Run Inference
    this->session->Run(
        options,
        inputNodeNames.data(),
        inputTensors.data(),
        inputTensors.size(),
        outputNodeNames.data(),
        outputTensors.data(),
        1);

    // Copy Output
    for(int i = 0; i < outputTensorSizes[0]; i++)
        output[i] = outputTensorValues[0][i];
}

// single input note
void OrtModel::run(float* input, float* output) 
{
    // Copy Inputs
    // Assume there is 1 input node 
    for(int i = 0; i < inputTensorSizes[0]; i++) 
        inputTensorValues[0][i] = input[i];

    // Run Inference
    this->session->Run(
        options,
        inputNodeNames.data(),
        inputTensors.data(),
        inputTensors.size(),
        outputNodeNames.data(),
        outputTensors.data(),
        1);

    // Copy Output
    for( int i = 0; i < outputTensorSizes[0]; i++)
        output[i] = outputTensorValues[0][i];
}

// single input node + cond params
void OrtModel::run(float* input, float* params, float* output) 
{
    // Copy Inputs
    // Assume there is 1 input node 
    for(int i = 0; i < inputTensorSizes[0]; i++)
        inputTensorValues[0][i] = input[i];

    // Copy Conditioning params
    for(int i = 0; i < inputTensorSizes[1]; i++)
        inputTensorValues[1][i] = params[i];

    // Run Inference
    this->session->Run(
        options,
        inputNodeNames.data(),
        inputTensors.data(),
        inputTensors.size(),
        outputNodeNames.data(),
        outputTensors.data(),
        1);

    // Copy Output
    for(int i = 0; i < outputTensorSizes[0]; i++)
        output[i] = outputTensorValues[0][i];
}

void OrtModel::cleanup()
{
    if(verbose)
        printf("Cleanup ONNX session\n");

    // Check if the session is initialized
    if (this->session != nullptr)
    {
    // Properly release the session
        delete this->session;
        this->session = nullptr;
    }

    // Clear out input-related vectors
    inputNodeNames.clear();
    inputNodeDims.clear();
    inputTensorSizes.clear();
    inputTensorValues.clear();

    // Clear tensors
    inputTensors.clear();
    outputTensors.clear();

    // Clear output-related vectors
    outputNodeNames.clear();
    outputNodeDims.clear();
    outputTensorSizes.clear();
    outputTensorValues.clear();

    // Reset other member variables
    numInputNodes = 0;
    numOutputNodes = 0;
    modelPath = nullptr;
    sessionName = nullptr;
}
