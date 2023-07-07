#ifndef OPACITY_H
#define OPACITY_H

#include "utils.h"
#include <atomic>
#include <map>
#include <mutex>
#include <set>
#include <vector>

struct DataItemInfo {
  int State = 0;
  int Ts;
  mylock M;
};

struct Transaction {
  int Id;
  TransStatus Status;
  std::map<int, std::pair<int, int>> LocalCopy; // DataItem id -> <state, ts>
  std::set<int> ReadSet;
  std::set<int> WriteSet;
  std::set<int> LockedSet;
  Transaction(int Id) : Id(Id) {}
};

class Scheduler {
private:
  FILE *LogFile;
  std::vector<DataItemInfo *> *SharedMemory;

public:
  std::atomic<int> CurrTransNum;
  std::atomic<int> Wts;

  Scheduler(std::vector<DataItemInfo *> *SharedMemory, FILE *LogFile)
      : SharedMemory(SharedMemory), LogFile(LogFile) { CurrTransNum = 0; }

  Transaction *begin_trans();
  int invoke(Transaction *T, Operation Op, unsigned X, int L);
  TransStatus tryCommit(Transaction *T);
  TransStatus tryAbort(Transaction *T);
  void reset(Transaction *T);
};
#endif