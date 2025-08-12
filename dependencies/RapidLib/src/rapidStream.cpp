/**
 * @file    rapidStream.cpp
 * @author  Michael Zbyszynski
 * @date    6 Feb 2017
 * @copyright Copyright © 2017 Goldsmiths. All rights reserved.
 */

#include "rapidStream.h"
#include <iostream>
#include <cmath>
#include <limits>
#include <numeric>

#ifdef EMSCRIPTEN
#include "emscripten/rapidStreamEmbindings.h"
#endif

namespace rapidLib
{

template<typename T>
rapidStream<T>::rapidStream(std::size_t window_size) : windowSize(window_size), windowIndex(0)
{
  circularWindow.resize(windowSize);
  std::fill(circularWindow.begin(), circularWindow.end(), 0);

  //Baysian Filter setup
  bayesFilt.diffusion = powf(10.0f, -2);
  bayesFilt.jump_rate = powf(10.0f, -10);
  bayesFilt.mvc[0] = 1.0f;
  bayesFilt.init();
}

template<typename T>
rapidStream<T>::rapidStream() : windowSize(3), windowIndex(0)
{
  circularWindow.resize(windowSize);
  std::fill(circularWindow.begin(), circularWindow.end(), 0);

  //Baysian Filter setup
  bayesFilt.diffusion = powf(10.0f, -2);
  bayesFilt.jump_rate = powf(10.0f, -10);
  bayesFilt.mvc[0] = 1.0;
  bayesFilt.init();
}

template<typename T>
rapidStream<T>::~rapidStream()
{
  //delete[]circularWindow;
}

template<typename T>
void rapidStream<T>::clear()
{
  windowIndex = 0;
  circularWindow.resize(windowSize);
  std::fill(circularWindow.begin(), circularWindow.end(), 0);
}

template<typename T>
void rapidStream<T>::pushToWindow(T input)
{
  circularWindow[windowIndex] = input;
  windowIndex = (windowIndex + 1) % windowSize;
}

template<typename T>
inline T rapidStream<T>::calcCurrentVel(std::size_t i) const
{
  return circularWindow[(i + windowIndex) % windowSize] - circularWindow[(i + windowIndex - 1) % windowSize];
}

template<typename T>
T rapidStream<T>::velocity() const
{
  return calcCurrentVel(-1);
};

template<typename T>
T rapidStream<T>::acceleration() const
{
  return calcCurrentVel(-2) - calcCurrentVel(-3);
};

template<typename T>
T rapidStream<T>::minimum() const
{
  return *std::min_element(circularWindow.begin(), circularWindow.end());
}

template<typename T>
T rapidStream<T>::maximum() const
{
  return *std::max_element(circularWindow.begin(), circularWindow.end());;
}

template<typename T>
uint32_t rapidStream<T>::numZeroCrossings() const
{
  uint32_t zeroCrossings {};

  //Is the begininng positive, negative, or 0?
  int previous { 1 };

  if (circularWindow[windowIndex] < 0)
  {
    previous = -1;
  }
  else if (circularWindow[windowIndex] == 0)
  {
    ++zeroCrossings;
    previous = 0;
  }

  for (std::size_t i { 1 }; i < windowSize; ++i)
  {
    const std::size_t index { (windowIndex + i) % windowSize};

    if (circularWindow[index] < 0 && previous >= 0) //Transition to negative
    {
      ++zeroCrossings;
      previous = -1;
    }
    else if (circularWindow[index] > 0 && previous <= 0) //Transition to positive
    {
      ++zeroCrossings;
      previous = 1;
    }
    else //Sample == 0
    {
      previous = 0;
    }
  }

  return zeroCrossings;
}

template<typename T>
T rapidStream<T>::sum() const
{
  return std::reduce(circularWindow.begin(), circularWindow.end());
}

template<typename T>
T rapidStream<T>::mean() const
{
  return sum() / windowSize;
}

template<typename T>
T rapidStream<T>::standardDeviation() const
{
  const T newMean { mean() };
  T standardDeviation {};

  for (const auto value : circularWindow)
  {
    standardDeviation += static_cast<T>(std::pow(value - newMean, 2));
  }

  return std::sqrt(standardDeviation / windowSize);
}

template<typename T>
T rapidStream<T>::rms() const
{
  T rms {};

  for (auto value:circularWindow)
  {
    rms += value * value;
  }

  rms = rms / windowSize;

  return std::sqrt(rms);
}

template<typename T>
T rapidStream<T>::bayesFilter(T input)
{
  std::vector<float> inputVec = { float(input) };
  bayesFilt.update(inputVec);
  return static_cast<T>(bayesFilt.output[0]);
}

template<typename T>
void rapidStream<T>::bayesSetDiffusion(float diffusion)
{
  bayesFilt.diffusion = powf(10., diffusion);
  bayesFilt.init();
}

template<typename T>
void rapidStream<T>::bayesSetJumpRate(float jump_rate)
{
  bayesFilt.jump_rate = powf(10., jump_rate);
  bayesFilt.init();
}

template<typename T>
void rapidStream<T>::bayesSetMVC(float mvc)
{
  bayesFilt.mvc[0] = mvc;
  bayesFilt.init();
}

template<typename T>
T rapidStream<T>::minVelocity() const
{
  T minVel { std::numeric_limits<T>::infinity() };

  for (std::size_t i {}; i < windowSize; ++i)
  {
    const T currentVel { calcCurrentVel(i) };
    if (currentVel < minVel)  minVel = currentVel;
  }

  return minVel;
}

template<typename T>
T rapidStream<T>::maxVelocity() const
{
  T maxVel { std::numeric_limits<T>::lowest() };

  for (std::size_t i {}; i < windowSize; ++i)
  {
    const T currentVel { calcCurrentVel(i) };
    if (currentVel > maxVel) maxVel = currentVel;
  }

  return maxVel;
}

template<typename T>
T rapidStream<T>::minAcceleration() const
{
  T minAccel { std::numeric_limits<T>::infinity() };
  T lastVel { calcCurrentVel(1) };

  for (std::size_t i { 2 }; i < windowSize; ++i)
  {
    const T currentVel { calcCurrentVel(i) };
    const T currentAccel { currentVel - lastVel };
    lastVel = currentVel;
    if (currentAccel < minAccel) minAccel = currentAccel;
  }

  return minAccel;
}

template<typename T>
T rapidStream<T>::maxAcceleration() const
{
  T maxAccel { std::numeric_limits<T>::lowest() };
  T lastVel { calcCurrentVel(1) };

  for (std::size_t i { 2 }; i < windowSize; ++i)
  {
    const T currentVel { calcCurrentVel(i) };
    const T currentAccel { currentVel - lastVel };
    lastVel = currentVel;
    if (currentAccel > maxAccel) maxAccel = currentAccel;
  }

  return maxAccel;
}

//explicit instantiation
template class rapidStream<double>;
template class rapidStream<float>;

}; //namespace rapidLib
