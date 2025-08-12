/**
 * @file regression.cpp
 * RapidLib
 *
 * @author Michael Zbsyzynski
 * @date 26 Sep 2016
 * @copyright Copyright © 2016 Goldsmiths. All rights reserved.
 */

#include <vector>
#include <iostream>
#include <time.h>
#include "regression.h"


#ifdef EMSCRIPTEN
#include "emscripten/regressionEmbindings.h"
#endif

template<typename T>
regressionTemplate<T>::regressionTemplate() :
numEpochs(500),
numHiddenLayers(1),
numHiddenNodes(0) //this will be changed by training
{
  modelSet<T>::numInputs = -1;
  modelSet<T>::numOutputs = -1;
  modelSet<T>::isTraining = false;
};

template<typename T>
regressionTemplate<T>::regressionTemplate(const int &num_inputs, const int &num_outputs) :
numEpochs(500),
numHiddenLayers(1),
numHiddenNodes(num_inputs)
{
  modelSet<T>::numInputs = num_inputs;
  modelSet<T>::numOutputs = num_outputs;
  modelSet<T>::isTraining = false;
  created = false;
  std::vector<size_t> whichInputs;
  
  for (int i {}; i < modelSet<T>::numInputs; ++i) whichInputs.push_back(i);
  
  for (int i {}; i < modelSet<T>::numOutputs; ++i)
  {
    modelSet<T>::myModelSet.push_back(new neuralNetwork<T>(modelSet<T>::numInputs, whichInputs, numHiddenLayers, numHiddenNodes));
  }
  
  created = true;
};

template<typename T>
regressionTemplate<T>::regressionTemplate(const std::vector<trainingExampleTemplate<T> > &training_set)
{
  modelSet<T>::numInputs = -1;
  modelSet<T>::numOutputs = -1;
  modelSet<T>::isTraining = false;
  train(training_set);
};

template<typename T>
std::vector<size_t> regressionTemplate<T>::getNumHiddenLayers() const
{
  std::vector<size_t> vecNumHiddenLayers {};
  
  if (std::begin(modelSet<T>::myModelSet) != std::end(modelSet<T>::myModelSet))
  {
    for (const baseModel<T>* model : modelSet<T>::myModelSet)
    {
      vecNumHiddenLayers.push_back(dynamic_cast<const neuralNetwork<T>*>(model)->getNumHiddenLayers()); //FIXME: I really dislike this design
    }
  }
  else
  {
    vecNumHiddenLayers = { numHiddenLayers };
  }
  
  return vecNumHiddenLayers;
}

template<typename T>
void regressionTemplate<T>::setNumHiddenLayers(const int &num_hidden_layers)
{
  numHiddenLayers = num_hidden_layers;
  //Set any existing models
  if (std::begin(modelSet<T>::myModelSet) != std::end(modelSet<T>::myModelSet))
  {
    for (baseModel<T>* model : modelSet<T>::myModelSet)
    {
      dynamic_cast<neuralNetwork<T>*>(model)->setNumHiddenLayers(num_hidden_layers); //FIXME: I really dislike this design
    }
  }
}

template<typename T>
std::vector<size_t> regressionTemplate<T>::getNumHiddenNodes() const
{
  std::vector<size_t> vecNumHiddenNodes {};
  
  if (std::begin(modelSet<T>::myModelSet) != std::end(modelSet<T>::myModelSet))
  {
    for (const baseModel<T>* model : modelSet<T>::myModelSet)
    {
      vecNumHiddenNodes.push_back(dynamic_cast<const neuralNetwork<T>*>(model)->getNumHiddenNodes()); //FIXME: I really dislike this design
    }
  }
  else
  {
    vecNumHiddenNodes = { numHiddenNodes };
  }
  
  return vecNumHiddenNodes;
}

template<typename T>
void regressionTemplate<T>::setNumHiddenNodes(const int &num_hidden_nodes)
{
  numHiddenNodes = num_hidden_nodes;
  
  //Set any existing models
  if (std::begin(modelSet<T>::myModelSet) != std::end(modelSet<T>::myModelSet))
  {
    for (baseModel<T>* model : modelSet<T>::myModelSet)
    {
      dynamic_cast<neuralNetwork<T>*>(model)->setNumHiddenNodes(num_hidden_nodes); //FIXME: I really dislike this design
    }
  }
}


template<typename T>
std::vector<size_t> regressionTemplate<T>::getNumEpochs() const
{
  std::vector<size_t> vecEpochs;
  if (std::begin(modelSet<T>::myModelSet) != std::end(modelSet<T>::myModelSet))
  {
    for (const baseModel<T>* model : modelSet<T>::myModelSet)
    {
      vecEpochs.push_back(dynamic_cast<const neuralNetwork<T>*>(model)->getEpochs());  //FIXME: I really dislike this design
    }
  }
  else
  {
    vecEpochs = { numEpochs };
  }
  return vecEpochs;
}

template<typename T>
void regressionTemplate<T>::setNumEpochs(const size_t &epochs)
{
  numEpochs = epochs;
  
  //set any existing models
  if (std::begin(modelSet<T>::myModelSet) != std::end(modelSet<T>::myModelSet))
  {
    for (baseModel<T>* model : modelSet<T>::myModelSet)
    {
      dynamic_cast<neuralNetwork<T>*>(model)->setEpochs(epochs); //FIXME: I really dislike this design
    }
  }
}

template<typename T>
bool regressionTemplate<T>::train(const std::vector<trainingExampleTemplate<T> > &training_set)
{
  //clock_t timer;
  //timer = clock();
  modelSet<T>::reset(); //FIXME: Should use modelSet if already created?
  if (training_set.size() > 0)
  {
    //create model(s) here
    modelSet<T>::numInputs = int(training_set[0].input.size());
    modelSet<T>::numOutputs = int(training_set[0].output.size());
    
    for (int i = 0; i < modelSet<T>::numInputs; ++i)
    {
      modelSet<T>::inputNames.push_back("inputs-" + std::to_string(i + 1));
    }
    modelSet<T>::numOutputs = int(training_set[0].output.size());
    
    for ( auto example : training_set)
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
    
    if(numHiddenNodes == 0) numHiddenNodes = modelSet<T>::numInputs;
    std::vector<size_t> whichInputs;
    
    for (int j = 0; j < modelSet<T>::numInputs; ++j)
    {
      whichInputs.push_back(j);
    }
    
    for (int i = 0; i < modelSet<T>::numOutputs; ++i)
    {
      modelSet<T>::myModelSet.push_back(new neuralNetwork<T>(modelSet<T>::numInputs, whichInputs, numHiddenLayers, numHiddenNodes));
    }
    
    if (numEpochs != 500)
    {
      for (baseModel<T>* model : modelSet<T>::myModelSet)
      {
        neuralNetwork<T>* nnModel = dynamic_cast<neuralNetwork<T>*>(model); //FIXME: I really dislike this design
        nnModel->setEpochs(numEpochs);
      }
    }
    
    //timer = clock() - timer;
    bool result = modelSet<T>::train(training_set);
    //std::cout << "Regression trained in " << (float)timer/CLOCKS_PER_SEC << " ms." << std::endl;
    return result;
  }
  throw std::length_error("empty training set.");
  return false;
}

template<typename T>
float regressionTemplate<T>::getTrainingProgress()
{
  float progress = modelSet<T>::isTrained ? 1.f : 0.f;
  
  if (modelSet<T>::isTraining)
  {
    for (baseModel<T>* model : modelSet<T>::myModelSet)
    {
      neuralNetwork<T>* nnModel = dynamic_cast<neuralNetwork<T>*>(model); //FIXME: I really dislike this design
      progress += (nnModel->getCurrentEpoch() / nnModel->getEpochs());
    }
    
    progress /= modelSet<T>::myModelSet.size();
  }
  
  return progress;
}

//explicit instantiation
template class regressionTemplate<double>;
template class regressionTemplate<float>;
