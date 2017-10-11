#include <map>
#include <vector>
#include <iostream>
#include <algorithm>
#include <cmath>

#include "Exception.hpp"
#include "TimeSeries.hpp"
#include "distance/Distance.hpp"

using std::string;
using std::vector;
using std::min;
using std::max;
using std::cout;
using std::endl;

namespace onex {

const dist_t getDistance(const string& distance_name)
{
  if (distance_name == "euclidean") {
    return pairwiseDistance;
  }
  else if (distance_name == "euclidean_dtw") {
    return warpedDistance;
  }
  throw OnexException(string("Cannot find distance with name: ") + distance_name);
}

data_t _euc(data_t x_1, data_t x_2)
{
  return pow(x_1 - x_2, 2);
}

data_t _euc_norm(data_t total, const TimeSeries& t_1, const TimeSeries& t_2)
{
  return sqrt(total / std::max(t_1.getLength(), t_2.getLength()));
}

data_t _euc_inorm(data_t dropout, const TimeSeries& t_1, const TimeSeries& t_2)
{
  return dropout * dropout * std::max(t_1.getLength(), t_2.getLength());
}

data_t _euc_norm_dtw(data_t total, const TimeSeries& t_1, const TimeSeries& t_2)
{
  return sqrt(total) / (2 * std::max(t_1.getLength(), t_2.getLength()));
}

data_t _euc_inorm_dtw(data_t dropout, const TimeSeries& t_1, const TimeSeries& t_2)
{
  return pow(dropout * 2 * std::max(t_1.getLength(), t_2.getLength()), 2);
}

data_t warpedDistance(const TimeSeries& a, const TimeSeries& b, data_t dropout)
{
  int m = a.getLength();
  int n = b.getLength();
  int r = calculateWarpingBandSize(max(m, n));
  data_t idropout = _euc_inorm_dtw(dropout, a, b);

  // Fastpath for base intervals
  if (m == 1 && n == 1)
  {
    return _euc_norm_dtw(_euc(a[0], b[0]), a, b);
  }

  // create cost matrix
  vector< vector< data_t >> cost(m, vector< data_t >(n));

  // Preset this in case this cell is never reached due to the warping
  // band constraint
  cost[m - 1][n - 1] = INF;
  cost[0][0] = _euc(a[0], b[0]);

  // calculate first column
  for(int i = 1; i < min(2*r + 1, m); i++)
  {
    cost[i][0] = cost[i-1][0] + _euc(a[i], b[0]);
  }

  // calculate first row
  for(int j = 1; j < min(2*r + 1, n); j++)
  {
    cost[0][j] = cost[0][j-1] + _euc(a[0], b[j]);
  }

  data_t result;
  bool dropped = false;
  for(int i = 1; i < m; i++)
  {
    data_t bestSoFar = INF;
    for(int j = max(i - r, 1); j <= min(i + r, n - 1); j++)
    {
      data_t ij1  = (i - r <= j-1 && j-1 <= i + r) ? cost[i][j-1] : INF;
      data_t i1j1 = cost[i-1][j-1];
      data_t i1j  = (j - r <= i-1 && i-1 <= j + r) ? cost[i-1][j] : INF;
      data_t minPrev = cost[i-1][j];
      if (i1j1 < ij1 && i1j1 < i1j)
      {
        minPrev = cost[i-1][j-1];
      }
      else if (ij1 < i1j)
      {
        minPrev = cost[i][j-1];
      }
      cost[i][j] = minPrev + _euc(a[i], b[j]);
      bestSoFar = min(bestSoFar, cost[i][j]);
    }

    if (bestSoFar > idropout)
    {
      dropped = true;
      break;
    }
  }
  result = dropped ? INF : cost[m - 1][n - 1];
  return _euc_norm_dtw(result, a, b);
}

double warpingBandRatio = 0.1;

void setWarpingBandRatio(double ratio) {
  warpingBandRatio = ratio;
}

int calculateWarpingBandSize(int length)
{
  int bandSize = floor(length * warpingBandRatio);
  return std::min(bandSize, length - 1);
}

data_t kimLowerBound(const TimeSeries& a, const TimeSeries& b, data_t dropout)
{
  int al = a.getLength();
  int bl = b.getLength();
  int l = min(al, bl);

  if (l == 0) {
    return 0;
  }

  if (l == 1) {
    return _euc(a[0], b[0]);
  }

  double lb = 0;

  lb += _euc(a[0], b[0]);
  lb += _euc(a[al - 1], b[bl - 1]);
  if (lb > dropout) {
    return INF;
  }

  lb += min(min(_euc(a[0], b[1]),
                _euc(a[1], b[1])),
                _euc(a[1], b[0]));
  if (lb > dropout) {
    return INF;
  }

  lb += min(min(_euc(a[al-1], b[bl-2]),
                _euc(a[al-2], b[bl-2])),
                _euc(a[al-2], b[bl-1]));
  if (lb >=dropout) {
    return INF;
  }

  if (l == 4) {
    return lb;
  }

  lb += min(min(min(_euc(a[0], b[2]),
                    _euc(a[1], b[2])),
                min(_euc(a[2], b[2]),
                    _euc(a[2], b[1]))),
            _euc(a[2], b[0]));

  if (lb > dropout) {
    return INF;
  }

  lb += min(min(min(_euc(a[al-1], b[bl-3]),
                    _euc(a[al-2], b[bl-3])),
                min(_euc(a[al-3], b[bl-3]),
                    _euc(a[al-3], b[bl-2]))),
            _euc(a[al-3], b[bl-1]));
  return lb;
}

data_t keoghLowerBound(const TimeSeries& a, const TimeSeries& b, data_t dropout)
{

  int len = min(a.getLength(), b.getLength());
  int warpingBand = calculateWarpingBandSize(max(a.getLength(), b.getLength()));
  const data_t* aLower = a.getKeoghLower(warpingBand);
  const data_t* aUpper = a.getKeoghUpper(warpingBand);
  data_t idropout = dropout * 2 * max(a.getLength(), b.getLength());
  idropout *= idropout;
  data_t lb = 0;

  for (int i = 0; i < len && lb < idropout; i++)
  {
    if (b[i] > aUpper[i]) {
      lb += _euc(b[i], aUpper[i]);
    }
    else if(b[i] < aLower[i]) {
      lb += _euc(b[i], aLower[i]);
    }
  }
  return _euc_norm_dtw(lb, a, b);
}

data_t crossKeoghLowerBound(const TimeSeries& a, const TimeSeries& b, data_t dropout)
{
  data_t lb = keoghLowerBound(a, b, dropout);
  if (lb > dropout) {
    return INF;
  }
  else {
    return max(lb, keoghLowerBound(b, a, dropout));
  }
}

data_t cascadeDistance(const TimeSeries& a, const TimeSeries& b, data_t dropout)
{
  // Temporarily disable this because the code seems to be problematic
  // data_t lb = kimLowerBound(a, b, dropout);
  // if (lb > dropout) {
  //   return INF;
  // }
  data_t lb = crossKeoghLowerBound(a, b, dropout);
  if (lb > dropout) {
    return INF;
  }
  data_t d = warpedDistance(a, b, dropout);
  return d;
}

data_t pairwiseDistance(const TimeSeries& x_1, const TimeSeries& x_2, data_t dropout)
{
  if (x_1.getLength() != x_2.getLength())
  {
    throw OnexException("Two time series must have the same length for pairwise distance");
  }

  data_t total = 0;

  bool dropped = false;
  dropout = _euc_inorm(dropout, x_1, x_2);

  for(int i = 0; i < x_1.getLength(); i++)
  {
    total += _euc(x_1[i], x_2[i]);
    if (total > dropout)
    {
      dropped = true;
      break;
    }
  }
  data_t result = dropped ? INF : _euc_norm(total, x_1, x_2);
  return result;
}


} // namespace onex
