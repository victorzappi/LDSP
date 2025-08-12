//
//  classification.h
//  RapidLib
//
//  Created by mzed on 26/09/2016.
//  Copyright © 2016 Goldsmiths. All rights reserved.
//


#include <vector>
#include "classification.h"
#ifdef EMSCRIPTEN
#include "emscripten/classificationEmbindings.h"
#endif

template<typename T>
classificationTemplate<T>::classificationTemplate()
{
  modelSet<T>::numInputs = -1;
  modelSet<T>::numOutputs = -1;
  modelSet<T>::isTraining = false;
  classificationType = knn; //this is the default algorithm
};

template<typename T>
classificationTemplate<T>::classificationTemplate(classificationTypes classification_type)
{
  modelSet<T>::numInputs = -1;
  modelSet<T>::numOutputs = -1;
  modelSet<T>::isTraining = false;
  classificationType = classification_type;
};

template<typename T>
classificationTemplate<T>::classificationTemplate(const int &num_inputs, const int &num_outputs) //TODO: this feature isn't really useful
{
  modelSet<T>::numInputs = num_inputs;
  modelSet<T>::numOutputs = num_outputs;
  modelSet<T>::isTraining = false;
  std::vector<size_t> whichInputs;
  
  for (size_t i = 0; i < modelSet<T>::numInputs; ++i)
  {
    whichInputs.push_back(i);
  }
  std::vector<trainingExampleTemplate<T> > trainingSet;
  
  for (size_t i = 0; i < modelSet<T>::numOutputs; ++i)
  {
    modelSet<T>::myModelSet.push_back(new knnClassification<T>(modelSet<T>::numInputs, whichInputs, trainingSet, 1));
  }
};

template<typename T>
classificationTemplate<T>::classificationTemplate(const std::vector<trainingExampleTemplate<T> > &trainingSet)
{
  modelSet<T>::numInputs = -1;
  modelSet<T>::numOutputs = -1;
  modelSet<T>::isTraining = false;
  train(trainingSet);
};

template<typename T>
bool classificationTemplate<T>::train(const std::vector<trainingExampleTemplate<T> > &training_set)
{
  //TODO: time this process?
  modelSet<T>::reset();
  
  if (training_set.size() > 0)
  {
    //create model(s) here
    modelSet<T>::numInputs = static_cast<int>(training_set[0].input.size());
    modelSet<T>::numOutputs = static_cast<int>(training_set[0].output.size());

    for (int i {}; i < modelSet<T>::numInputs; ++i)
    {
      modelSet<T>::inputNames.push_back("inputs-" + std::to_string(i + 1));
    }

    modelSet<T>::numOutputs = static_cast<int>(training_set[0].output.size());

    for (const auto& example : training_set)
    {
      if (example.input.size() != modelSet<T>::numInputs)
      {
        throw std::length_error("unequal feature vectors in input.");
        return false;
      }

      if (example.output.size() != modelSet<T>::numOutputs)
      {
        throw std::length_error("unequal output vectors.");
        return false;
      }
    }

    std::vector<size_t> whichInputs {};

    for (int inputNum {}; inputNum < modelSet<T>::numInputs; ++inputNum)
    {
      whichInputs.push_back(inputNum);
    }
    
    for (int outputNum {}; outputNum < modelSet<T>::numOutputs; ++outputNum)
    {
      if (classificationType == svm)
      {
        modelSet<T>::myModelSet.push_back(new svmClassification<T>(modelSet<T>::numInputs));
      }
      else
      {
        modelSet<T>::myModelSet.push_back(new knnClassification<T>(modelSet<T>::numInputs, whichInputs, training_set, 1));
      }
    }
    
    return modelSet<T>::train(training_set);
  }
  return false;
}

template<typename T>
std::vector<int> classificationTemplate<T>::getK()
{
  std::vector<int> kVector {};

  for (const baseModel<T>* model : modelSet<T>::myModelSet)
  {
    kVector.push_back(dynamic_cast<const knnClassification<T>*>(model)->getK()); //FIXME: I really dislike this design
  }

  return kVector;
}

template<typename T>
void classificationTemplate<T>::setK(const int whichModel, const int newK)
{
  if (modelSet<T>::myModelSet.size() > whichModel)
  {
    dynamic_cast<knnClassification<T>*>(modelSet<T>::myModelSet[whichModel])->setK(newK); //FIXME: I really dislike this design
  }
  else
  {
    throw std::length_error("model not in model set.");
  }
}

//explicit instantiation
template class classificationTemplate<double>;
template class classificationTemplate<float>;
