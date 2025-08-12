/**
 * @file neuralNetwork.cpp
 *  RapidLib
 *
 * @date 05 Sep 2016
 * @copyright Copyright © 2016 Goldsmiths. All rights reserved.
 */

#include <math.h>
#include <random>
#include <algorithm>
#include <vector>

#include "neuralNetwork.h"
#ifdef EMSCRIPTEN
#include "emscripten/nnEmbindings.h"
#endif

template<typename T>
void neuralNetwork<T>::initTrainer()
{
  //initialize deltas
  //FIXME: This creates a vector of numHiddenLayers x numHiddenNodes x numInputs.  It fails between hidden vectors if numHiddenNodes > numInputs.
  //This hacky fix makes it too big if there are more hidden nodes. Shouldn't crash, though.
  if (numHiddenNodes > numInputs)
  {
    deltaWeights = std::vector<std::vector<std::vector<T> > >(numHiddenLayers, std::vector<std::vector<T> >(numHiddenNodes, std::vector<T>((numHiddenNodes + 1), 0)));
  }
  else
  {
    deltaWeights = std::vector<std::vector<std::vector<T> > >(numHiddenLayers, std::vector<std::vector<T> >(numHiddenNodes, std::vector<T>((numInputs + 1), 0)));
  }
  deltaHiddenOutput = std::vector<T>((numHiddenNodes + 1), 0);
}

/*!
 * This is the constructor for a model imported from JSON.
 */

template<typename T>
neuralNetwork<T>::neuralNetwork(const size_t& num_inputs,
                                const std::vector<size_t>& which_inputs,
                                const size_t& num_hidden_layers,
                                const size_t& num_hidden_nodes,
                                const std::vector<T>& _weights,
                                const std::vector<T>& w_hidden_output,
                                const std::vector<T>& in_ranges,
                                const std::vector<T>& in_bases,
                                const T& out_range,
                                const T& out_base
                                )
:
numInputs(num_inputs),
whichInputs(which_inputs),
numHiddenLayers(num_hidden_layers),
numHiddenNodes(num_hidden_nodes),
wHiddenOutput(w_hidden_output),
inRanges(in_ranges),
inBases(in_bases),
outRange(out_range),
outBase(out_base),
outputErrorGradient(0)
{
  bool randomize { _weights.size() ? false : true };
  std::default_random_engine generator;
  std::uniform_real_distribution<T> distribution(-0.5, 0.5);

  //winding up a long vector from javascript
  size_t count {};
  for (size_t i {}; i < numHiddenLayers; ++i)
  {
    std::vector<std::vector<T>> layer;
    for (size_t j {}; j < numHiddenNodes; ++j)
    {
      std::vector<T> node;
      size_t numConnections = (i == 0) ? numInputs : numHiddenNodes;
      for (size_t k = 0; k <= numConnections; ++k)
      {
        if (randomize)
        {
          node.push_back(distribution(generator));
        }
        else {
          node.push_back(_weights[count]);
        }
        count++;
      }
      layer.push_back(node);
    }
    weights.push_back(layer);
  }

  if (randomize)
  {
    for (size_t i {}; i <= numHiddenNodes; ++i)
    {
      wHiddenOutput.push_back( distribution(generator) );
    }
  }

  for(auto inRange : inRanges)
  {
    if (inRange == 0.)
    {
      inRange = 1.0; //Prevent divide by zero later.
    }
  }

  //trainer -- do we really need this?
  initTrainer();

  // setup neurons
  inputLayer.resize(numInputs);
  inputLayer.push_back(1.0); // last neuron is always 1, to apply layer bias
  hiddenLayers.resize(numHiddenLayers);
  for (auto& layer : hiddenLayers)
  {
    layer.resize(numHiddenNodes + 1); // extra node for layer bias
  }
}

/*!
 * This is the constructor for a model that needs to be trained.
 */

template<typename T>
neuralNetwork<T>::neuralNetwork(const size_t& num_inputs,
                                const std::vector<size_t>& which_inputs,
                                const size_t& num_hidden_layers,
                                const size_t& num_hidden_nodes
                                )
:
numInputs(num_inputs),
whichInputs(which_inputs),
numHiddenLayers(num_hidden_layers),
numHiddenNodes(num_hidden_nodes),
outputErrorGradient(0)
{
  //randomize weights
  reset();

  //trainer
  initTrainer();

  // setup neurons
  inputLayer.resize(numInputs);
  inputLayer.push_back(1.0); // layer bias
  hiddenLayers.resize(numHiddenLayers);
  for (auto& layer : hiddenLayers)
  {
    layer.resize(numHiddenNodes + 1); // extra node for layer bias
  }
}

/*!
 * This destructor is not needed.
 */

template<typename T>
neuralNetwork<T>::~neuralNetwork()
{
}

template<typename T>
void neuralNetwork<T>::reset()
{
  std::default_random_engine generator;
  std::uniform_real_distribution<T> distribution(-0.5, 0.5);

  weights.clear();
  for (size_t i {}; i < numHiddenLayers; ++i)
  {
    std::vector<std::vector<T>> layer;
    for (size_t j{}; j < numHiddenNodes; ++j)
    {
      std::vector<T> node;
      size_t numConnections = (i == 0) ? numInputs : numHiddenNodes;
      for (size_t k {}; k <= numConnections; ++k)
      {
        node.push_back(distribution(generator));
      }
      layer.push_back(node);
    }
    weights.push_back(layer);
  }

  wHiddenOutput.clear();

  for (size_t i {}; i <= numHiddenNodes; ++i)
  {
    wHiddenOutput.push_back(distribution(generator));
  }
}

template<typename T>
inline T neuralNetwork<T>::getHiddenErrorGradient(size_t layer, size_t neuron)
{
  T weightedSum {};

  if (numHiddenLayers == 1 || layer == 0)
  {
    T wGradient = wHiddenOutput[neuron] * outputErrorGradient;
    return hiddenLayers[layer][neuron] * (1 - hiddenLayers[layer][neuron]) * wGradient;
  }

  if (layer == numHiddenLayers - 1)
  {
    for (size_t i {}; i < numHiddenNodes; ++i)
    {
      weightedSum += wHiddenOutput[i] * outputErrorGradient;
    }
  }
  else
  {
    for (size_t i {}; i < numHiddenNodes; ++i)
    {
      weightedSum += deltaWeights[layer + 1][neuron][i] * outputErrorGradient;
    }
  }
  return hiddenLayers[layer][neuron] * (1 - hiddenLayers[layer][neuron]) * weightedSum;
}

template<typename T>
inline T neuralNetwork<T>::activationFunction(T x)
{
  if (x < -45) x = 0; //from weka, to combat overflow
  else if (x > 45) x = 1;
  else x = 1 / (1 + exp(-x)); //sigmoid
  return x;
}

template<typename T>
size_t neuralNetwork<T>::getNumInputs() const
{
  return numInputs;
}

template<typename T>
std::vector<size_t> neuralNetwork<T>::getWhichInputs() const
{
  return whichInputs;
}

template<typename T>
size_t neuralNetwork<T>::getNumHiddenLayers() const
{
  return numHiddenLayers;
}

template<typename T>
void neuralNetwork<T>::setNumHiddenLayers(size_t num_hidden_layers)
{
  numHiddenLayers = num_hidden_layers;
  reset();
  initTrainer();
}

template<typename T>
size_t neuralNetwork<T>::getNumHiddenNodes() const
{
  return numHiddenNodes;
}

template<typename T>
void neuralNetwork<T>::setNumHiddenNodes(size_t num_hidden_nodes)
{
  numHiddenNodes = num_hidden_nodes;
  reset();
  initTrainer();
}

template<typename T>
size_t neuralNetwork<T>::getEpochs() const
{
  return numEpochs;
}

template<typename T>
void neuralNetwork<T>::setEpochs(const size_t& epochs)
{
  numEpochs = epochs;
}

template<typename T>
std::vector<T> neuralNetwork<T>::getWeights() const
{
  std::vector<T> flatWeights;
  for (auto weightsA : weights)
  {
    for (auto weightsB : weightsA)
    {
      for (auto weightC : weightsB)
      {
        flatWeights.push_back(weightC);
      }
    }
  }
  return flatWeights;
}

template<typename T>
std::vector<T> neuralNetwork<T>::getWHiddenOutput() const
{
  return wHiddenOutput;
}

template<typename T>
std::vector<T> neuralNetwork<T>::getInRanges() const
{
  return inRanges;
}

template<typename T>
std::vector<T> neuralNetwork<T>::getInBases() const
{
  return inBases;
}

template<typename T>
T neuralNetwork<T>::getOutRange() const
{
  return outRange;
}

template<typename T>
T neuralNetwork<T>::getOutBase() const
{
  return outBase;
}

#ifndef EMSCRIPTEN
template<typename T>
void neuralNetwork<T>::getJSONDescription(Json::Value& jsonModelDescription)
{
  jsonModelDescription["modelType"] = "Neural Network";
  jsonModelDescription["numInputs"] = (int)numInputs;  //FIXME: Update json::cpp?
  jsonModelDescription["whichInputs"] = this->vector2json(whichInputs);
  jsonModelDescription["numHiddenLayers"] = (int)numHiddenLayers;
  jsonModelDescription["numHiddenNodes"] = (int)numHiddenNodes; //FIXME: Update json::cpp?
  jsonModelDescription["numHiddenOutputs"] = 1;
  jsonModelDescription["inRanges"] = this->vector2json(inRanges);
  jsonModelDescription["inBases"] = this->vector2json(inBases);
  jsonModelDescription["outRange"] = outRange;
  jsonModelDescription["outBase"] = outBase;

  //Create Nodes
  Json::Value nodes;

  //Output Node
  Json::Value outNode;
  outNode["name"] = "Linear Node 0";

  for (size_t i {}; i < numHiddenNodes; ++i)
  {
    std::string nodeName = "Node " + std::to_string(i + 1);
    outNode[nodeName] = wHiddenOutput[i];
  }

  outNode["Threshold"] = wHiddenOutput[numHiddenNodes];
  nodes.append(outNode);

  //Input nodes
  for (size_t i {}; i < weights.size(); ++i)
  { //layers
    for (size_t j {}; j < weights[i].size(); ++j)
    { //hidden nodes
      Json::Value tempNode;
      tempNode["name"] = "Sigmoid Node " + std::to_string((i * numHiddenNodes) + j + 1);
      for (size_t k {}; k < weights[i][j].size() - 1; ++k)
      { //inputs + threshold aka bias
        std::string connectNode = "Attrib inputs-" + std::to_string(k + 1);
        tempNode[connectNode] = weights[i][j][k];
      }
      tempNode["Threshold"] = weights[i][j][weights[i][j].size() - 1];
      nodes.append(tempNode);
    }
  }

  jsonModelDescription["nodes"] = nodes;
}
#endif

template<typename T>
T neuralNetwork<T>::run(const std::vector<T>& inputVector)
{
  std::vector<T> pattern;
  for (size_t h {}; h < numInputs; h++)
  {
    pattern.push_back(inputVector[whichInputs[h]]);
  }

  //set input layer
  for (size_t i {}; i < numInputs; ++i)
  {
    inputLayer[i] = (pattern[i] - inBases[i]) / inRanges[i];
  }

  //calculate hidden layers
  //for (size_t layerNum {}; auto& layer : hiddenLayers) //VIC remove C++ 20 feature
  size_t layerNum = 0;
  for (auto& layer : hiddenLayers)
  {
    for (size_t j {}; j < numHiddenNodes; ++j)
    {
      layer[j] = 0;

      const auto& previousLayer { layerNum == 0 ? inputLayer : hiddenLayers[layerNum - 1] };

      //for (size_t k {}; auto& input : previousLayer) //VIC remove C++ 20 feature
      size_t k = 0;
      for (auto& input : previousLayer)
      {
        layer[j] += input * weights[layerNum][j][k];
        ++k;
      }

      layer[j] = activationFunction(layer[j]);
    }

    layer.back() = 1.0; //for bias weight
    ++layerNum;
  }

  //calculate output
  outputNeuron = 0;
  // for (size_t i {}; auto& hiddenNeuron : hiddenLayers.back()) //VIC remove C++ 20 feature
  size_t i = 0;
  for (auto& hiddenNeuron : hiddenLayers.back())
  {
    outputNeuron += hiddenNeuron * wHiddenOutput[i];
    ++i;
  }

  //if classifier, outputNeuron = activationFunction(outputNeuron), else...
  outputNeuron = (outputNeuron * outRange) + outBase;
  return outputNeuron;
}

template<typename T>
void neuralNetwork<T>::train(const std::vector<trainingExampleTemplate<T > >& trainingSet)
{
  train(trainingSet, 0);
}


template<typename T>
void neuralNetwork<T>::train(const std::vector<trainingExampleTemplate<T > >& trainingSet, const std::size_t whichOutput)
{
  initTrainer();
  //setup maxes and mins
  std::vector<T> inMax = trainingSet[0].input;
  std::vector<T> inMin = trainingSet[0].input;
  T outMin = trainingSet[0].output[whichOutput];
  T outMax = trainingSet[0].output[whichOutput];

  for(auto trainingExample : trainingSet)
  {
    for (size_t i {}; i < numInputs; ++i)
    {
      if (trainingExample.input[i] > inMax[i]) inMax[i] = trainingExample.input[i];
      if (trainingExample.input[i] < inMin[i]) inMin[i] = trainingExample.input[i];
      if (trainingExample.output[whichOutput] > outMax) outMax = trainingExample.output[whichOutput];
      if (trainingExample.output[whichOutput] < outMin) outMin = trainingExample.output[whichOutput];
    }
  }
  inRanges.clear();
  inBases.clear();

  for (size_t i {}; i < numInputs; ++i)
  {
    inRanges.push_back((inMax[i] - inMin[i]) * 0.5);
    inBases.push_back((inMax[i] + inMin[i]) * 0.5);
  }

  for (auto inRange : inRanges)
  {
    if (inRange == 0.) inRange = 1.0; //Prevent divide by zero later.
  }
  outRange = (outMax - outMin) * (T)0.5;
  outBase = (outMax + outMin) * (T)0.5;

  //train
  if (outRange) //Don't need to do any training if output never changes
  {
    for (currentEpoch = 0; currentEpoch < numEpochs; ++currentEpoch)
    {
      //run through every training instance
      for (auto trainingExample : trainingSet)
      {
        run(trainingExample.input);
        backpropagate(trainingExample.output[whichOutput]);
      }
    }
  }
}

template<typename T>
void neuralNetwork<T>::backpropagate(const T& desiredOutput)
{
  outputErrorGradient = ((desiredOutput - outBase) / outRange) - ((outputNeuron - outBase) / outRange); //FIXME: could be tighter -MZ

  //correction based on size of last layer. Is this right? -MZ
  T length = 0;
  for (size_t i {}; i < numHiddenNodes; ++i)
  {
    length += hiddenLayers[numHiddenLayers - 1][i] * hiddenLayers[numHiddenLayers - 1][i];
  }
  if (length <= 2.0) length = 1.0;

  //deltas between hidden and output
  for (size_t i {}; i <= numHiddenNodes; ++i)
  {
    deltaHiddenOutput[i] = (learningRate * (hiddenLayers[numHiddenLayers - 1][i] / length) * outputErrorGradient) + (momentum * deltaHiddenOutput[i]);
  }

  //deltas between hidden
  for (int i { static_cast<int>(numHiddenLayers) - 1 }; i >= 0; --i)
  {
    for (size_t j {}; j < numHiddenNodes; ++j)
    {
      T hiddenErrorGradient = getHiddenErrorGradient(i, j);
      if (i > 0)
      {
        for (size_t k {}; k <= numHiddenNodes; ++k)
        {
          deltaWeights[i][j][k] = (learningRate * hiddenLayers[i][j] * hiddenErrorGradient) + (momentum * deltaWeights[i][j][k]);
        }
      }
      else //hidden to input layer
      {
        for (size_t k {}; k <= numInputs; ++k)
        {
          deltaWeights[0][j][k] = (learningRate * inputLayer[k] * hiddenErrorGradient) + (momentum * deltaWeights[0][j][k]);
        }

      }
    }
  }
  updateWeights();
}

template<typename T>
void neuralNetwork<T>::updateWeights()
{
  //hidden to hidden weights
  for (int i {}; i < numHiddenLayers; ++i)
  {
    size_t numDeltas { (i == 0) ? numInputs : numHiddenNodes };
    for (size_t j {}; j < numHiddenNodes; ++j)
    {
      for (size_t k = 0; k <= numDeltas; ++k)
      {
        weights[i][j][k] += deltaWeights[i][j][k];
      }
    }
  }
  //hidden to output weights
  for (size_t i {}; i <= numHiddenNodes; ++i)
  {
    wHiddenOutput[i] += deltaHiddenOutput[i];
  }
}

template<typename T>
size_t neuralNetwork<T>::getCurrentEpoch() const
{
  return currentEpoch;
}

//explicit instantiation
template class neuralNetwork<double>;
template class neuralNetwork<float>;
