#include "OnexAPI.hpp"

#include "Exception.hpp"
#include "GroupableTimeSeriesSet.hpp"
#include "distance/Distance.hpp"

using std::string;
using std::vector;

namespace onex {

OnexAPI::~OnexAPI()
{
  unloadAllDataset();
}

dataset_info_t OnexAPI::loadDataset(const string& filePath, int maxNumRow,
                                     int startCol, const string& separators)
{

  GroupableTimeSeriesSet* newSet = new GroupableTimeSeriesSet();
  try {
    newSet->loadData(filePath, maxNumRow, startCol, separators);
  } catch (OnexException& e)
  {
    delete newSet;
    throw e;
  }

  int nextIndex = -1;
  for (unsigned int i = 0; i < this->loadedDatasets.size(); i++)
  {
    if (this->loadedDatasets[i] == nullptr)
    {
      nextIndex = i;
      break;
    }
  }

  if (nextIndex < 0) {
    nextIndex = this->loadedDatasets.size();
    this->loadedDatasets.push_back(nullptr);
  }

  this->loadedDatasets[nextIndex] = newSet;
  this->datasetCount++;

  return this->getDatasetInfo(nextIndex);
}

void OnexAPI::saveDataset(int index, const string& filePath, char separator)
{
  this->_checkDatasetIndex(index);
  this->loadedDatasets[index]->saveData(filePath, separator);
}

void OnexAPI::unloadDataset(int index)
{
  this->_checkDatasetIndex(index);

  delete loadedDatasets[index];
  loadedDatasets[index] = nullptr;
  if (index == loadedDatasets.size() - 1)
  {
    loadedDatasets.pop_back();
  }
  this->datasetCount--;
}

void OnexAPI::unloadAllDataset()
{
  for (unsigned int i = 0; i < this->loadedDatasets.size(); i++)
  {
    delete this->loadedDatasets[i];
  }
  this->loadedDatasets.clear();
  this->datasetCount = 0;
}

int OnexAPI::getDatasetCount()
{
  return this->datasetCount;
}

dataset_info_t OnexAPI::getDatasetInfo(int index)
{
  this->_checkDatasetIndex(index);

  GroupableTimeSeriesSet* dataset = this->loadedDatasets[index];
  return dataset_info_t(index,
                        dataset->getFilePath(),
                        dataset->getItemCount(),
                        dataset->getItemLength(),
                        dataset->isGrouped(),
                        dataset->isNormalized());
}

vector<dataset_info_t> OnexAPI::getAllDatasetInfo()
{
  vector<dataset_info_t> info;
  for (unsigned int i = 0; i < this->loadedDatasets.size(); i++)
  {
    if (loadedDatasets[i] != nullptr)
    {
      info.push_back(getDatasetInfo(i));
    }
  }
  return info;
}

std::pair<data_t, data_t> OnexAPI::normalizeDataset(int idx)
{
  this->_checkDatasetIndex(idx);
  return this->loadedDatasets[idx]->normalize();
}

int OnexAPI::groupDataset(int index, data_t threshold)
{
  this->_checkDatasetIndex(index);
  return this->loadedDatasets[index]->groupAllLengths("euclidean", threshold);
}

void OnexAPI::saveGroup(int index, const string &path, bool groupSizeOnly)
{
  this->_checkDatasetIndex(index);
  this->loadedDatasets[index]->saveGroups(path, groupSizeOnly);
}

int OnexAPI::loadGroup(int index, const string& path)
{
  this->_checkDatasetIndex(index);
  return this->loadedDatasets[index]->loadGroups(path);
}

void OnexAPI::setWarpingBandRatio(double ratio)
{
  onex::setWarpingBandRatio(ratio);
}

candidate_time_series_t OnexAPI::getBestMatch(int result_idx, int query_idx, int index, int start, int end)
{
  this->_checkDatasetIndex(result_idx);
  this->_checkDatasetIndex(query_idx);

  const TimeSeries& query = loadedDatasets[query_idx]->getTimeSeries(index, start, end);
  return loadedDatasets[result_idx]->getBestMatch(query);
}

dataset_info_t OnexAPI::PAA(int idx, int n)
{
  this->_checkDatasetIndex(idx);
  this->loadedDatasets[idx]->PAA(n);
  return this->getDatasetInfo(idx);
}


void OnexAPI::_checkDatasetIndex(int index)
{
  if (index < 0 || index >= loadedDatasets.size() || loadedDatasets[index] == nullptr)
  {
    throw OnexException("There is no dataset with given index");
  }
}

} // namespace onex
