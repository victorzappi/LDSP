#include "OrtModel.h"
#include <iostream>
#include <thread>

#include <numeric> // std::accumulate()

Ort::MemoryInfo memoryInfo = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
Ort::RunOptions options;

template <typename T>
T vectorProduct(const std::vector<T> &v) {
  return std::accumulate(v.begin(), v.end(), 1, std::multiplies<T>());
}

bool OrtModel::setup(const char * _sessionName, const char * _modelPath, bool _multiThreading) {

    this->modelPath = _modelPath;
    this->sessionName = _sessionName;


    Ort::SessionOptions sessionOptions;
    Ort::AllocatorWithDefaultOptions allocator;

    sessionOptions.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
    sessionOptions.EnableCpuMemArena();

    if (_multiThreading) {
        unsigned int max_threads = std::thread::hardware_concurrency();
        printf("Max thread: %d\n", max_threads);
        sessionOptions.SetIntraOpNumThreads(max_threads);
    }

    // Load Model
    this->env = new Ort::Env(OrtLoggingLevel::ORT_LOGGING_LEVEL_WARNING, _sessionName);
    this->session = new Ort::Session(*env, _modelPath, sessionOptions);



    printf("\nLoaded Model!!\n");
    // Get number of inputs/outputs to the model
    numInputNodes = this->session->GetInputCount();
    numOutputNodes = this->session->GetOutputCount();    

    printf("Model Dimensions:\n");
    printf("---Input: %zu\n", numInputNodes);
    printf("---Output: %zu\n", numOutputNodes);


    // allocate space to hold names of model inputs
    inputNodeNames.resize(numInputNodes);
    inputNodeDims.resize(numInputNodes);
    // Gather metadata information about the model outputs
    for (int i = 0; i < numInputNodes; i++) {

        printf("==================================\n");
        
        // Get names of each input node
        Ort::AllocatedStringPtr inputName = session->GetInputNameAllocated(i, allocator);
        printf("Input %d : name=%s\n", i, inputName.get());
        inputNodeNames[i] = inputName.get();
        inputName.release();

        // Get Data type of each input node
        Ort::TypeInfo type_info = session->GetInputTypeInfo(i);
        auto tensorInfo = type_info.GetTensorTypeAndShapeInfo();

        ONNXTensorElementDataType type = tensorInfo.GetElementType();
        printf("Input %d : type=%d\n", i, type);

        // Get shapes of input tensors
        inputNodeDims[i] = tensorInfo.GetShape();
        printf("Input %d : num_dims=%zu\n", i, inputNodeDims.size());
        for (int j = 0; j < inputNodeDims.size(); j++) {
            printf("Input %d : dim %d=%lld\n", i, j, inputNodeDims[i][j]);
            if ((int) this->inputNodeDims[i][j] < 0) {
                printf("Input %d, dim %d has a variable size, forcing a size of 1", i,j);
                inputNodeDims[i][j] = inputNodeDims[i][j]*-1;
            }
        }

        inputTensorSizes.push_back(vectorProduct(inputNodeDims[i]));
        inputTensorValues.push_back(std::vector<float>(inputTensorSizes[i]));
    }


    // allocate space to hold names of model outputs
    outputNodeNames.resize(numOutputNodes);
    outputNodeDims.resize(numOutputNodes);
    // Gather metadata information about the model outputs
    for (int i = 0; i < numOutputNodes; i++) {

        printf("==================================\n");
        
        // Get names of each output node
        Ort::AllocatedStringPtr outputName = session->GetOutputNameAllocated(i, allocator);
        printf("Output %d : name=%s\n", i, outputName.get());
        outputNodeNames[i] = outputName.get();
        outputName.release();

        // Get Data type of each output node
        Ort::TypeInfo typeInfo = session->GetOutputTypeInfo(i);
        auto tensorInfo = typeInfo.GetTensorTypeAndShapeInfo();

        ONNXTensorElementDataType type = tensorInfo.GetElementType();
        printf("Output %d : type=%d\n", i, type);

        // Get shapes of output tensors
        outputNodeDims[i] = tensorInfo.GetShape();
        printf("Output %d : num_dims=%zu\n", i, outputNodeDims.size());
        for (int j = 0; j < outputNodeDims.size(); j++) {
            printf("Output %d : dim %d=%lld\n", i, j, outputNodeDims[i][j]);
            if ((int) this->outputNodeDims[i][j] < 0) {
                printf("Output %d, dim %d has a variable size, forcing a size of 1", i,j);
                outputNodeDims[i][j] = outputNodeDims[i][j]*-1;
            }
        }

        outputTensorSizes.push_back(vectorProduct(outputNodeDims[i]));
        outputTensorValues.push_back(std::vector<float>(outputTensorSizes[i]));
    }

    printf("\n");

    for (int i = 0; i < numInputNodes; i++) {
        inputTensors.push_back(Ort::Value::CreateTensor<float>(
                                memoryInfo,
                                inputTensorValues[i].data(),
                                inputTensorValues[i].size(),
                                inputNodeDims[i].data(),
                                inputNodeDims[i].size()));
    }
    for (int i = 0; i < numOutputNodes; i++) {
        outputTensors.push_back(Ort::Value::CreateTensor<float>(
            memoryInfo,
            outputTensorValues[i].data(),
            outputTensorValues[i].size(),
            outputNodeDims[i].data(),
            outputNodeDims[i].size()));
    }

    return true;   
}

// void OrtModel::run(std::vector<std::vector<float>> inputs, float * output) {

//     // Copy Inputs
//     for (int i = 0; i < numInputNodes; i++) {
//         for (int j = 0; j < inputTensorSizes[i]; j++) {
//             inputTensorValues[i][j] = inputs[i][j];
//         }
//     }

//     // Run Inference
//     this->session->Run(
//         options,
//         inputNodeNames.data(),
//         inputTensors.data(),
//         inputTensors.size(),
//         outputNodeNames.data(),
//         outputTensors.data(),
//         1);

//     // Copy Output
//     for (int i = 0; i < outputTensorSizes[0]; i++) {
//         output[i] = outputTensorValues[0][i];
//     }
// }


void OrtModel::run(float ** inputs, float * output) {

    // Copy Inputs
    for (int i = 0; i < numInputNodes; i++) {
        for (int j = 0; j < inputTensorSizes[i]; j++) {
            inputTensorValues[i][j] = inputs[i][j];
        }
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
    for (int i = 0; i < outputTensorSizes[0]; i++) {
        output[i] = outputTensorValues[0][i];
    }
}

void OrtModel::run(float * input, float * output) {

    // Copy Inputs
    // Assume there is 1 input node 
    for (int i = 0; i < inputTensorSizes[0]; i++) {
        inputTensorValues[0][i] = input[i];
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
    for (int i = 0; i < outputTensorSizes[0]; i++) {
        output[i] = outputTensorValues[0][i];
    }
}

void OrtModel::run(float * input, float * params, float * output) {

    // Copy Inputs
    // Assume there is 1 input node 
    for (int i = 0; i < inputTensorSizes[0]; i++) {
        inputTensorValues[0][i] = input[i];
    }

    for (int i = 0; i < inputTensorSizes[1]; i++) {
        inputTensorValues[1][i] = params[i];
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
    for (int i = 0; i < outputTensorSizes[0]; i++) {
        output[i] = outputTensorValues[0][i];
    }
}
