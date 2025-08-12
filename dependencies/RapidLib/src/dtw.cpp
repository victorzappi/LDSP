/**
 * @file dtw.cpp
 * RapidLib
 *
 * @author Michael Zbyszynski
 * @date 07 Jun 2017
 * @copyright Copyright © 2017 Goldsmiths. All rights reserved.
 */

#include <vector>
#include <cmath>
#include <limits>
#include <stdexcept>
#include "dtw.h"

template<typename T>
dtw<T>::dtw() {};

template<typename T>
dtw<T>::~dtw() {};

template<typename T>
inline T dtw<T>::distanceFunction(const std::vector<T> &x, const std::vector<T> &y)
{
  double euclidianDistance {};

  if (x.size() != y.size())
  {
    throw std::length_error("comparing different length series");
  }
  else
  {
    for (std::size_t i {}; i < x.size(); ++i)
    {
      euclidianDistance += std::pow((x[i] - y[i]), 2);
    }
    
    euclidianDistance = sqrt(euclidianDistance);
  }

  return static_cast<T>(euclidianDistance);
};

/* Just returns the cost, doesn't calculate the path */
template<typename T>
T dtw<T>::getCost(const std::vector<std::vector<T>>& seriesX, const std::vector<std::vector<T>>& seriesY)
{
  if (seriesX.size() < seriesY.size()) return getCost(seriesY, seriesX);
  
  costMatrix.clear();
  for (size_t framesX {}; framesX < seriesX.size(); ++framesX) costMatrix.push_back(std::vector<T>(seriesY.size(), 0));
  
  const std::size_t maxX { seriesX.size() - 1 };
  const std::size_t maxY { seriesY.size() - 1 };
  
  //Calculate values for the first column
  costMatrix[0][0] = distanceFunction(seriesX[0], seriesY[0]);
  for (int y { 1 }; y <= maxY; ++y)
  {
    costMatrix[0][y] = costMatrix[0][y - 1] + distanceFunction(seriesX[0], seriesY[y]);
  }
  
  for (std::size_t x { 1 }; x <= maxX; ++x)
  {
    //Bottom row of current column
    costMatrix[x][0] = costMatrix[x - 1][0] + distanceFunction(seriesX[x], seriesY[0]);

    for (std::size_t y { 1 }; y <= maxY; ++y)
    {
      T minGlobalCost { (std::min(costMatrix[x - 1][y - 1], costMatrix[x][y - 1])) };
      costMatrix[x][y] = minGlobalCost + distanceFunction(seriesX[x], seriesY[y]);
    }
  }

  return costMatrix[maxX][maxY];
};

template<typename T>
warpPath dtw<T>::calculatePath(std::size_t seriesXsize, std::size_t seriesYsize) const
{
  warpPath warpPath;
  std::size_t x { seriesXsize - 1 };
  std::size_t y { seriesYsize - 1 };
  warpPath.add(x, y);

  while ((x > 0) || (y > 0))
  {
    T diagonalCost { ((x > 0) && (y > 0)) ? costMatrix[x - 1][y - 1] : std::numeric_limits<T>::infinity() };
    T leftCost { (x > 0) ? costMatrix[x - 1][y] : std::numeric_limits<T>::infinity() };
    T downCost { (y > 0) ? costMatrix[x][y - 1] : std::numeric_limits<T>::infinity() };

    if ((diagonalCost <= leftCost) && (diagonalCost <= downCost))
    {
      if (x > 0) --x;
      if (y > 0) --y;
    }
    else if ((leftCost < diagonalCost) && (leftCost < downCost))
    {
      --x;
    }
    else if ((downCost < diagonalCost) && (downCost < leftCost))
    {
      --y;
    }
    else if (x <= y)
    {
      --y;
    }
    else
    {
      --x;
    }
    warpPath.add(x, y);
  }
  
  return warpPath;
};

/* calculates both the cost and the warp path*/
template<typename T>
warpInfo<T> dtw<T>::dynamicTimeWarp(const std::vector<std::vector<T>>& seriesX, const std::vector<std::vector<T>>& seriesY)
{
  warpInfo<T> info {};

  //calculate cost matrix
  info.cost = getCost(seriesX, seriesY);
  info.path = calculatePath(seriesX.size(), seriesY.size());
  return info;
}

/* calculates warp info based on window */
template<typename T>
warpInfo<T> dtw<T>::constrainedDTW(const std::vector<std::vector<T> > &seriesX, const std::vector<std::vector<T> > &seriesY, searchWindow<T> window)
{
  //initialize cost matrix
  costMatrix.clear();
  std::vector<T> tempVector(seriesY.size(), std::numeric_limits<T>::max());
  costMatrix.assign(seriesX.size(), tempVector); //TODO: this could be smaller, since most cells are unused
  std::size_t maxX { seriesX.size() - 1 };
  std::size_t maxY { seriesY.size() - 1 };

  //fill cost matrix cells based on window
  for (std::size_t currentX {}; currentX < window.minMaxValues.size(); ++currentX)
  {
    for (std::size_t currentY { window.minMaxValues[currentX].first }; currentY <= window.minMaxValues[currentX].second; ++currentY) //FIXME: should be <= ?
    {
      if (currentX == 0 && currentY == 0)  //bottom left cell
      {
        costMatrix[0][0] = distanceFunction(seriesX[0], seriesY[0]);
      }
      else if (currentX == 0) //first column
      {
        costMatrix[0][currentY] = distanceFunction(seriesX[0], seriesY[currentY]) + costMatrix[0][currentY - 1];
      }
      else if (currentY == 0) //first row
      {
        costMatrix[currentX][0] = distanceFunction(seriesX[currentX], seriesY[0]) + costMatrix[currentX - 1][0];
      }
      else
      {
        T minGlobalCost { std::min(costMatrix[currentX - 1][currentY], std::min(costMatrix[currentX-1][currentY-1], costMatrix[currentX][currentY-1])) };
        costMatrix[currentX][currentY] = distanceFunction(seriesX[currentX], seriesY[currentY]) + minGlobalCost;
      }
    }
  }
  
  warpInfo<T> info;
  info.cost = costMatrix[maxX][maxY];
  info.path = calculatePath(seriesX.size(), seriesY.size());
  return info;
}

//explicit instantiation
template class dtw<double>;
template class dtw<float>;
