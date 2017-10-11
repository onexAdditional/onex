#ifndef GENERAL_DISTANCE_H
#define GENERAL_DISTANCE_H

#include <vector>
#include <algorithm>
#include <iostream>
#include "TimeSeries.hpp"
#include "Exception.hpp"

using std::min;
using std::max;
using std::make_pair;
using std::string;
using std::vector;
using std::pair;

namespace onex {

typedef data_t (*dist_t)(const TimeSeries&, const TimeSeries&, data_t);

int calculateWarpingBandSize(int length);
void setWarpingBandRatio(double ratio);
  
/**
 *  @brief returns the an object representing a distance metric
 *
 *  @param distance_name name of a distance metric
 *  @return an object containing methods of the requested distance metric
 *  @throw OnexException if no distance with given name is found
 */
const dist_t getDistance(const string& distance_name);

/**
 *  @return a vector of names of available distances
 */
const vector<string>& getAllDistanceName();

/**
 *  @brief returns the warped distance between two sets of data
 *
 *  @param metric the distance metric to use
 *  @param a one of the two arrays of data
 *  @param b the other of the two arrays of data
 *  @param dropout drops the calculation of distance if within this
 */
data_t warpedDistance(const TimeSeries& a, const TimeSeries& b, data_t dropout);

/**
 * Calculates pairwise distance between two time series. This function is enabled if the given
 * distance metric class DM has the 'hasInverseNorm' function.
 */
data_t pairwiseDistance(const TimeSeries& x_1, const TimeSeries& x_2, data_t dropout);

/**
 * ...
 */
data_t keoghLowerBound(const TimeSeries& a, const TimeSeries& b, data_t dropout);
data_t kimLowerBound(const TimeSeries& a, const TimeSeries& b, data_t dropout);
data_t crossKeoghLowerBound(const TimeSeries& a, const TimeSeries& b, data_t dropout);

/**
 * ...
 */
data_t cascadeDistance(const TimeSeries& a, const TimeSeries& b, data_t dropout);

} // namespace onex

#endif //GENERAL_DISTANCE_H
