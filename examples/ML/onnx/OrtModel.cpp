#include "OrtModel.h"
#include <iostream>


bool OrtModel::setup(const char * _sessionName, const char * _modelPath) {


    Ort::SessionOptions sessionOptions;
    sessionOptions.SetIntraOpNumThreads(1);

    this->modelPath = _modelPath;
    this->sessionName = _sessionName;
    this->env = new Ort::Env(OrtLoggingLevel::ORT_LOGGING_LEVEL_WARNING, _sessionName);
    this->session = new Ort::Session(*env, _modelPath, sessionOptions);

    Ort::AllocatorWithDefaultOptions allocator;


    printf("\nLoaded Model!!\n");
    // Get number of inputs/outputs to the model
    numInputNodes = this->session->GetInputCount();
    numOutputNodes = this->session->GetOutputCount();    

    printf("Model Dimensions:\n");
    printf("---Input: %zu\n", numInputNodes);
    printf("---Output: %zu\n", numOutputNodes);


    // allocate space to hold names of model inputs
    inputNodeNames.resize(numInputNodes);
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
        inputNodeDims = tensorInfo.GetShape();
        printf("Input %d : num_dims=%zu\n", i, inputNodeDims.size());
        for (int j = 0; j < inputNodeDims.size(); j++) {
            printf("Input %d : dim %d=%lld\n", i, j, inputNodeDims[j]);
        }
    }


    // allocate space to hold names of model outputs
    outputNodeNames.resize(numOutputNodes);
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
        outputNodeDims = tensorInfo.GetShape();
        printf("Output %d : num_dims=%zu\n", i, outputNodeDims.size());
        for (int j = 0; j < outputNodeDims.size(); j++) {
            printf("Output %d : dim %d=%lld\n", i, j, outputNodeDims[j]);
        }
    }

    printf("\n");

    return true;   
}


void OrtModel::run() {

    size_t inputTensorSize = 1; 

    // dummy input values to send to model
    std::vector<float> inputTensorValues(inputTensorSize);

    // fill dummy input values
    inputTensorValues[0] = 0.5;

    // create input tensor object from data values
    auto memoryInfo = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
    Ort::Value inputTensor = Ort::Value::CreateTensor<float>(memoryInfo, inputTensorValues.data(), inputTensorSize, inputNodeDims.data(), 2);

    Ort::RunOptions options;
    std::vector<Ort::Value> outputTensors = session->Run(options, inputNodeNames.data(), &inputTensor, 1, outputNodeNames.data(), 1);

    printf("ran model sucessfully !!! \n");

    for (int i = 0; i < outputTensors.size(); i++) {
        float * tensor = (float *) outputTensors[i].GetTensorRawData();
        printf("============================\n");
        printf("Output %d : %f", i, tensor[0]);
    }

}

void OrtModel::run(float * inputTensorData, float * outputTensorData) {


    // create input tensor object from data values
    auto memoryInfo = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
    Ort::Value inputTensor = Ort::Value::CreateTensor<float>(memoryInfo, inputTensorData, this->inputNodeDims[1], inputNodeDims.data(), 2);

    Ort::RunOptions options;
    std::vector<Ort::Value> outputTensors = session->Run(options, inputNodeNames.data(), &inputTensor, 1, outputNodeNames.data(), 1);


    // std::vector<float> outputData = outputTensors[0].getTensorRawData();
    float * tensorData = (float *) outputTensors[0].GetTensorRawData();
    memcpy(outputTensorData, tensorData, sizeof(float)*this->outputNodeDims[1]);
}