//
//  knnClassification.cpp
//  RapidLib
//
//  Created by mzed on 05/09/2016.
//  Copyright © 2016 Goldsmiths. All rights reserved.
//

#include "knnClassification.h"
#include <algorithm>
#include <map>
#include <math.h>
#include <utility>
#include <vector>

#include <cmath> //VIC no need in C++ 17

#ifdef EMSCRIPTEN
#include "emscripten/knnEmbindings.h"
#endif

template <typename T>
knnClassification<T>::knnClassification(
    const int &num_inputs,
    const std::vector<size_t> &which_inputs,
    const std::vector<trainingExampleTemplate<T>> &_neighbours,
    const int k)
    : numInputs(num_inputs),
      whichInputs(which_inputs),
      whichOutput(0),
      neighbours(_neighbours),
      desiredK(k),
      currentK(k)
{}

template <typename T> knnClassification<T>::~knnClassification() {}

template <typename T> void knnClassification<T>::reset()
{
  // TODO: implement this
}

template <typename T> size_t knnClassification<T>::getNumInputs() const
{
  return numInputs;
}

template <typename T>
std::vector<size_t> knnClassification<T>::getWhichInputs() const
{
  return whichInputs;
}

template <typename T> int knnClassification<T>::getK() const
{
  return currentK;
}

template <typename T> inline void knnClassification<T>::updateK()
{
  if (currentK != desiredK) currentK = std::min(desiredK, (int)neighbours.size());
}

template <typename T> void knnClassification<T>::setK(int newK)
{
  desiredK = newK;
  updateK();
}

template <typename T>
void knnClassification<T>::addNeighbour(const int classNum, const std::vector<T>& features)
{
  std::vector<T> classVec;
  classVec.push_back(T(classNum));
  trainingExampleTemplate<T> newNeighbour = {features, classVec};
  neighbours.push_back(newNeighbour);
  updateK();
};

template <typename T>
void knnClassification<T>::train(const std::vector<trainingExampleTemplate<T>>& trainingSet)
{
  train(trainingSet, 0);
}

// FIXME: Not paying attention to whichOutput.
template <typename T>
void knnClassification<T>::train(const std::vector<trainingExampleTemplate<T>>& trainingSet, const std::size_t which_output) // FIXME: Does numInputs need to be reset here? -MZ
{
  neighbours.clear();
  neighbours = trainingSet;
  updateK();
  whichOutput = which_output;
};

template <typename T>
T knnClassification<T>::run(const std::vector<T> &inputVector)
{
  std::vector<std::pair<int, T>> nearestNeighbours { static_cast<size_t>(currentK), std::make_pair(0, 0.0) }; // These are our k nearest neighbours
  std::pair<int, T> farthestNN{ 0, 0.0 }; // This one will be replaced if there's a closer one
  std::vector<T> pattern {}; // This is what we're trying to match

  for (const auto input : whichInputs)
  {
    pattern.push_back(inputVector[input]);
  }

  // Find k nearest neighbours
  //for (size_t index {}; const auto &neighbour : neighbours) //VIC remove C++ 20 feature
  size_t index = 0;
  for (const auto &neighbour : neighbours)
  {
    // find Euclidian distance for this neighbor
    T euclidianDistance {};

    //for (size_t j {}; j < numInputs; ++j) //VIC remove C++ 20 feature
    for (size_t j = 0; j < numInputs; ++j)
    {
      euclidianDistance += (T)std::pow((pattern[j] - neighbour.input[j]), 2);
    }

    euclidianDistance = std::sqrt(euclidianDistance);

    if (index < currentK)
    {
      // save the first k neighbours
      nearestNeighbours[index] = {index, euclidianDistance};
      if (euclidianDistance > farthestNN.second) farthestNN = { index, euclidianDistance };
    }
    else if (euclidianDistance < farthestNN.second)
    {
      // replace farthest, if new neighbour is closer
      nearestNeighbours[farthestNN.first] = {index, euclidianDistance};
      size_t currentFarthest{0};
      T currentFarthestDistance{0.0};

      for (size_t n {}; n < currentK; ++n)
      {
        if (nearestNeighbours[n].second > currentFarthestDistance)
        {
          currentFarthest = n;
          currentFarthestDistance = nearestNeighbours[n].second;
        }
      }
      farthestNN = { currentFarthest, currentFarthestDistance };
    }
    ++index;
  }

  // majority vote on nearest neighbours
  std::map<T, int> classVoteMap;
  using classVotePair = std::pair<int, int>;

  for (size_t i {}; i < currentK; ++i)
  {
    T classNum{ (T)round(neighbours[nearestNeighbours[i].first].output[whichOutput]) };

    if (classVoteMap.find(classNum) == classVoteMap.end())
    {
      classVoteMap.insert(classVotePair(classNum, 1));
    }
    else
    {
      ++classVoteMap[classNum];
    }
  }

  T foundClass {};
  int mostVotes {};

  for (const auto& classVote : classVoteMap)
  {
    if (classVote.second > mostVotes)
    {
      mostVotes = classVote.second;
      foundClass = classVote.first;
    }
  }

  return foundClass;
}

#ifndef EMSCRIPTEN
template <typename T>
void knnClassification<T>::getJSONDescription(Json::Value &jsonModelDescription)
{
  jsonModelDescription["modelType"] = "kNN Classificiation";
  jsonModelDescription["numInputs"] = numInputs;
  jsonModelDescription["whichInputs"] = this->vector2json(whichInputs);
  jsonModelDescription["k"] = desiredK;
  Json::Value examples;

  for (const auto& neighbour : neighbours)
  {
    Json::Value oneExample;
    oneExample["class"] = neighbour.output[whichOutput];
    oneExample["features"] = this->vector2json(neighbour.input);
    examples.append(oneExample);
  }

  jsonModelDescription["examples"] = examples;
}
#endif

// explicit instantiation
template class knnClassification<double>;
template class knnClassification<float>;
