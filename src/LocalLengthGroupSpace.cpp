#include "LocalLengthGroupSpace.hpp"

#include <vector>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <chrono>

#include "TimeSeries.hpp"
#include "Group.hpp"
#include "Exception.hpp"
#include "distance/Distance.hpp"

using std::cout;
using std::ofstream;
using std::ifstream;
using std::endl;

#define LOG_EVERY_S 10
#define LOG_FREQ  5

namespace onex {

LocalLengthGroupSpace::LocalLengthGroupSpace(const TimeSeriesSet& dataset, int length)
 : dataset(dataset), length(length)
{
  this->subTimeSeriesCount = dataset.getItemLength() - length + 1;
  this->memberMap = std::vector<group_membership_t>(dataset.getItemCount() * this->subTimeSeriesCount);
}

LocalLengthGroupSpace::~LocalLengthGroupSpace()
{
  reset();
}

void LocalLengthGroupSpace::reset()
{
  for (unsigned int i = 0; i < groups.size(); i++)
  {
    delete groups[i];
    groups[i] = nullptr;
  }
  groups.clear();
}

std::chrono::time_point<std::chrono::system_clock> _last_time;

int LocalLengthGroupSpace::generateGroups(const dist_t pairwiseDistance, data_t threshold)
{
  std::chrono::duration<float> elapsed_seconds = std::chrono::system_clock::now() - _last_time;
  bool doLog = false;
  if (elapsed_seconds.count() >= LOG_EVERY_S) {
    doLog = true;
    _last_time = std::chrono::system_clock::now();
  }
  if (doLog) {
    cout << "Processing time series space of length " << this->length << endl;
  }
  int totalTimeSeries = this->subTimeSeriesCount * dataset.getItemCount();
  int counter = 0;
  for (int start = 0; start < this->subTimeSeriesCount; start++)
  {
    for (int idx = 0; idx < dataset.getItemCount(); idx++)
    {
      counter++;
      if (doLog) {
        if (counter % (totalTimeSeries / LOG_FREQ) == 0) {
          cout << "  Grouping progress... " << counter << "/" << totalTimeSeries 
               << " (" << counter*100/totalTimeSeries << "%)" << endl;
        }
      }

      TimeSeries query = dataset.getTimeSeries(idx, start, start + this->length);

      data_t bestSoFar = INF;
      int bestSoFarIndex;

      for (unsigned int i = 0; i < groups.size(); i++)
      {
        data_t dist = this->groups[i]->distanceFromCentroid(query, pairwiseDistance, bestSoFar);
        if (dist < bestSoFar)
        {
          bestSoFar = dist;
          bestSoFarIndex = i;
        }
      }

      if (bestSoFar > threshold / 2 || this->groups.size() == 0)
      {
        bestSoFarIndex = this->groups.size();
        int newGroupIndex = this->groups.size();
        this->groups.push_back(new Group(newGroupIndex, this->length, this->subTimeSeriesCount,
                                         this->dataset, this->memberMap));
        this->groups[bestSoFarIndex]->setCentroid(idx, start);
      }

      this->groups[bestSoFarIndex]->addMember(idx, start);
    }
  }

  return this->getNumberOfGroups();
}

int LocalLengthGroupSpace::getNumberOfGroups(void) const
{
  return this->groups.size();
}

const Group* LocalLengthGroupSpace::getGroup(int idx) const
{
  if (idx < 0 || idx >= this->getNumberOfGroups()) {
    throw OnexException("Group index is out of range");
  }
  return this->groups[idx];
}

void LocalLengthGroupSpace::saveGroups(ofstream &fout, bool groupSizeOnly) const 
{
  // Number of groups having time series of this length
  fout << groups.size() << endl;
  if (groupSizeOnly) {
    for (unsigned int i = 0; i < groups.size(); i++) 
    {
      fout << groups[i]->getCount() << " ";
    }
    fout << endl;
  }
  else 
  {
    for (unsigned int i = 0; i < groups.size(); i++) 
    {
      groups[i]->saveGroup(fout);
    }
  }
}

int LocalLengthGroupSpace::loadGroups(ifstream &fin)
{
  reset();
  int numberOfGroups;
  fin >> numberOfGroups;
  for (unsigned int i = 0; i < numberOfGroups; i++)
  {
    Group* grp = new Group(i, this->length, this->subTimeSeriesCount, this->dataset, this->memberMap);
    grp->loadGroup(fin);
    this->groups.push_back(grp);
  }
  return numberOfGroups;
}

candidate_group_t LocalLengthGroupSpace::getBestGroup(const TimeSeries& query,
  const dist_t warpedDistance,
  data_t dropout) const
{
  data_t bestSoFarDist = dropout;
  const Group* bestSoFarGroup = nullptr;
  for (unsigned int i = 0; i < groups.size(); i++) {
    data_t dist = groups[i]->distanceFromCentroid(query, warpedDistance, bestSoFarDist);
    if (dist < bestSoFarDist) {
      bestSoFarDist = dist;
      bestSoFarGroup = groups[i];
    }
  }

  return std::make_pair(bestSoFarGroup, bestSoFarDist);
}

} // namespace onex