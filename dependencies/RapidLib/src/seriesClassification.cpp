/**
 *  @file seriesClassification.cpp
 *  RapidLib
 *
 *  @author Michael Zbyszynski
 *  @date 08 Jun 2017
 *  @copyright Copyright © 2017 Goldsmiths. All rights reserved.
 */

#include <vector>
#include <limits>
#include <algorithm>
#include <thread>
#include <stdexcept>

#include "seriesClassification.h"

#ifdef EMSCRIPTEN
#include "emscripten/seriesClassificationEmbindings.h"
#endif

static constexpr std::size_t SEARCH_RADIUS { 1 };

template<typename T>
seriesClassificationTemplate<T>::seriesClassificationTemplate() : isTraining(false) , hopSize(1), counter(0) {};

template<typename T>
seriesClassificationTemplate<T>::~seriesClassificationTemplate() {};

template<typename T>
bool seriesClassificationTemplate<T>::train(const std::vector<trainingSeriesTemplate<T> > &seriesSet) 
{
  bool success { false };
  
  if (isTraining)
  {
    throw std::runtime_error("model already training");
  }
  else if (seriesSet.size() <= 0)
  {
    throw std::length_error("training on empty training set.");
  }
  else
  {
    isTraining = true;
    reset();
    vectorLength = seriesSet[0].input[0].size(); //TODO: check that all vectors are the same size
    allTrainingSeries = seriesSet;
    minLength = maxLength = allTrainingSeries[0].input.size();
    
    for (const auto trainingSeries : allTrainingSeries)
    {
      //Global
      std::size_t newLength { trainingSeries.input.size() };
      if (newLength < minLength) minLength = newLength;
      if (newLength > maxLength) maxLength = newLength;
      
      //Per Label
      typename std::map<std::string, minMax<size_t> >::iterator it { lengthsPerLabel.find(trainingSeries.label) };
      if (it != lengthsPerLabel.end())
      {
        std::size_t newLength { trainingSeries.input.size() };
        if (newLength < it->second.min) it->second.min = newLength;
        if (newLength > it->second.max) it->second.max = newLength;
      }
      else
      {
        minMax<size_t> tempLengths;
        tempLengths.min = tempLengths.max = trainingSeries.input.size();
        lengthsPerLabel[trainingSeries.label] = tempLengths;
      }
    }
    
    //TODO: make this size smarter?
    std::vector<T> zeroVector(vectorLength, 0.0);
    seriesBuffer.clear();
    seriesBuffer.resize(minLength, zeroVector);
    isTraining = false;
    success = true;
  }
  return success;
};

template<typename T>
void seriesClassificationTemplate<T>::reset() 
{
  allCosts.clear();
  allTrainingSeries.clear();
  lengthsPerLabel.clear();
  minLength = -1;
  maxLength = -1;
  isTraining = false;
}

template<typename T>
std::string seriesClassificationTemplate<T>::run(const std::vector<std::vector<T>>& inputSeries)
{
  std::string returnLabel { "none" };
  if (isTraining)
  {
    throw std::runtime_error("can't run a model during training");
  }
  else if (allTrainingSeries.size() > 0)
  {
    std::size_t closestSeries { 0 };
    allCosts.clear();
    T lowestCost { fastDTW<T>::getCost(inputSeries, allTrainingSeries[0].input, SEARCH_RADIUS) };
    allCosts.push_back(lowestCost);
    
    for (std::size_t i { 1 }; i < allTrainingSeries.size(); ++i) 
    {
      T currentCost = fastDTW<T>::getCost(inputSeries, allTrainingSeries[i].input, SEARCH_RADIUS);
      allCosts.push_back(currentCost);
      if (currentCost < lowestCost) 
      {
        lowestCost = currentCost;
        closestSeries = i;
      }
    }
    returnLabel = allTrainingSeries[closestSeries].label;
  }
  return returnLabel;
};

template<typename T>
T seriesClassificationTemplate<T>::run(const std::vector< std::vector<T> >& inputSeries, std::string label)
{
  T returnValue = 0;
  
  if (isTraining)
  {
    throw std::runtime_error("can't run a model during training");
  }
  else
  {
    allCosts.clear();
    T lowestCost { std::numeric_limits<T>::max() };
    
    for (const auto trainingSeries : allTrainingSeries)
    {
      if (trainingSeries.label == label) 
      {
        const T currentCost { fastDTW<T>::getCost(inputSeries, trainingSeries.input, SEARCH_RADIUS) };
        allCosts.push_back(currentCost);
        if (currentCost < lowestCost) 
        {
          lowestCost = currentCost;
        }
      }
    }
    returnValue = lowestCost;
  }
  
  return returnValue;
};

template<typename T>
std::string seriesClassificationTemplate<T>::runParallel(const std::vector< std::vector<T> >& inputSeries) 
{
  std::string returnLabel { "none" };
  
  if (isTraining)
  {
    throw std::runtime_error("can't run a model during training");
  }
  else
  {
    allCosts.clear();
    std::vector<std::thread> runningThreads;
    
    for (std::size_t i { 0 }; i < allTrainingSeries.size(); ++i) 
    {
      runningThreads.push_back(std::thread(&seriesClassificationTemplate<T>::runThread, this, inputSeries, i));
    }
    
    for (auto& thread : runningThreads)
    {
      thread.join();
    }
    returnLabel = allTrainingSeries[findClosestSeries()].label;
  }
  
  return returnLabel;
};

template<typename T>
T seriesClassificationTemplate<T>::runParallel(const std::vector< std::vector<T> > &inputSeries, std::string label)
{
  T returnValue { 0 };
  if (isTraining)
  {
    throw std::runtime_error("can't run a model during training");
  }
  else
  {
    allCosts.clear();
    std::vector<std::thread> runningThreads;
    int seriesIndex { 0 };
    
    for (std::size_t i {}; i < allTrainingSeries.size(); ++i)
    {
      if (allTrainingSeries[i].label == label) 
      {
        runningThreads.push_back(std::thread(&seriesClassificationTemplate<T>::runThread, this, inputSeries, seriesIndex));
        ++seriesIndex;
      }
    }
    
    /*
     for (std::size_t i = 0; i < runningThreads.size(); ++i) 
     {
     runningThreads.at(i).join(); //FIXME: not sure what's up here...
     }
     */
    
    for (auto& thread : runningThreads)
    {
      thread.join();
    }
    
    returnValue = allCosts.at(findClosestSeries());
  }
  return returnValue;
};

template<typename T>
std::size_t seriesClassificationTemplate<T>::findClosestSeries() const 
{
  auto lowestCost { std::min_element(allCosts.begin(), allCosts.end()) };
  return std::size_t(std::distance(allCosts.begin(), lowestCost));
}

template<typename T>
void seriesClassificationTemplate<T>::runThread(const std::vector<std::vector<T>> &inputSeries, std::size_t i) 
{
  allCosts.push_back(std::numeric_limits<T>::max()); //initialized cost
  allCosts[i] = fastDTW<T>::getCost(inputSeries, allTrainingSeries[i].input, SEARCH_RADIUS);
}

template<typename T>
std::string seriesClassificationTemplate<T>::runContinuous(const std::vector<T> &inputVector) 
{
  seriesBuffer.erase(seriesBuffer.begin());
  seriesBuffer.push_back(inputVector);
  std::string returnString { "none" };
  if ((counter % hopSize) == 0 ) 
  {
    if (isTraining)
    {
      throw std::runtime_error("can't run a model during training");
    } 
    returnString = run(seriesBuffer); //TODO: Have an option to run parallel here.
    counter = 0;
  }
  ++counter;
  return returnString;
}

template<typename T>
std::vector<T> seriesClassificationTemplate<T>::getCosts() const
{
  return allCosts;
}

template<typename T>
std::size_t seriesClassificationTemplate<T>::getMinLength() const
{
  return minLength;
}

template<typename T>
std::size_t seriesClassificationTemplate<T>::getMinLength(std::string label) const 
{
  std::size_t labelMinLength { 0 };
  typename std::map<std::string, minMax<size_t> >::const_iterator it { lengthsPerLabel.find(label) };
  if (it != lengthsPerLabel.end()) labelMinLength = it->second.min;
  return labelMinLength;
}

template<typename T>
std::size_t seriesClassificationTemplate<T>::getMaxLength() const 
{
  return maxLength;
}

template<typename T>
std::size_t seriesClassificationTemplate<T>::getMaxLength(std::string label) const 
{
  std::size_t labelMaxLength { std::numeric_limits<std::size_t>::max() };
  typename std::map<std::string, minMax<size_t> >::const_iterator it { lengthsPerLabel.find(label) };
  if (it != lengthsPerLabel.end()) labelMaxLength = it->second.max;
  return labelMaxLength;
}

template<typename T>
typename seriesClassificationTemplate<T>::template minMax<T> seriesClassificationTemplate<T>::calculateCosts(std::string label) const 
{
  minMax<T> calculatedMinMax = {0, 0};
  bool foundSeries { false };
  std::vector<T> labelCosts;
  
  for (size_t i { 0 }; i < (allTrainingSeries.size() - 1); ++i) //these loops are a little different than the two-label case
  { 
    if (allTrainingSeries[i].label == label) 
    {
      foundSeries = true;
      for (size_t j = (i + 1); j < allTrainingSeries.size(); ++j) 
      {
        if (allTrainingSeries[j].label == label) 
        {
          labelCosts.push_back(fastDTW<T>::getCost(allTrainingSeries[i].input, allTrainingSeries[j].input, SEARCH_RADIUS));
        }
      }
    }
  }
  
  if (foundSeries) 
  {
    auto minmax_result { std::minmax_element(std::begin(labelCosts), std::end(labelCosts)) };
    calculatedMinMax.min = *minmax_result.first;
    calculatedMinMax.max = *minmax_result.second;
  } 
  
  return calculatedMinMax;
}

template<typename T>
typename seriesClassificationTemplate<T>::template minMax<T> seriesClassificationTemplate<T>::calculateCosts(std::string label1, std::string label2) const {
  minMax<T> calculatedMinMax = {0, 0};
  bool foundSeries { false };
  std::vector<T> labelCosts;
  
  for (const auto series1 : allTrainingSeries)
  {
    if (series1.label == label1) 
    {
      for (auto series2 : allTrainingSeries)
      {
        if (series2.label == label2)
        {
          foundSeries = true;
          labelCosts.push_back(fastDTW<T>::getCost(series1.input, series2.input, SEARCH_RADIUS));
        }
      }
    }
  }
  
  if (foundSeries) 
  {
    auto minmax_result { std::minmax_element(std::begin(labelCosts), std::end(labelCosts)) };
    calculatedMinMax.min = *minmax_result.first;
    calculatedMinMax.max = *minmax_result.second;
  } 
  
  return calculatedMinMax;
}

//explicit instantiation
template class seriesClassificationTemplate<double>;
template class seriesClassificationTemplate<float>;


//
//std::vector<T> seriesClassification::getCosts(const std::vector<trainingExample> &trainingSet) 
//{
//    run(trainingSet);
//    return allCosts;
//}
