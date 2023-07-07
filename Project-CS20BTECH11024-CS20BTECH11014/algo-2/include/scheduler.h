#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "utils.h"
#include <atomic>
#include <map>
#include <set>
#include <shared_mutex>
#include <vector>

struct DataItemInfo {
  int Val;
  std::map<int, int> Depend; // <DataItem id -> int>
  std::shared_mutex M;

  DataItemInfo() { Val = 0; }
  DataItemInfo(const DataItemInfo &XInfo) {
    Val = XInfo.Val;
    Depend = XInfo.Depend;
  }
};

struct ThreadMemory {
  std::map<int, DataItemInfo *> LocalCopy;
  std::map<int, int> P_Depend; // <DataItem id -> int>
};

struct Transaction {
  int Id;
  TransStatus Status;
  std::set<int> ReadSet;
  std::set<int> WriteSet;
  std::map<int, int> T_Depend; // <DataItem id -> int>
  Transaction(int Id, const std::map<int, int> &P_Depend)
      : Id(Id), T_Depend(P_Depend) {}
};

class Scheduler {
private:
  FILE *LogFile;
  std::vector<DataItemInfo *> *SharedMemory;
  std::vector<ThreadMemory *> *ThrMem;

public:
  std::atomic<int> CurrTransNum;

  Scheduler(std::vector<DataItemInfo *> *SharedMemory,
            std::vector<ThreadMemory *> *ThrMem, FILE *LogFile)
      : SharedMemory(SharedMemory), ThrMem(ThrMem), LogFile(LogFile) {
    CurrTransNum = 0;
  }

  Transaction *begin_trans(int ThId);
  void read(Transaction *T, unsigned X, int &L, int ThId);
  void write(Transaction *T, unsigned X, const int L, int ThId);
  TransStatus tryCommit(Transaction *T, int ThId);
  void garbageCollect(Transaction *T, int ThId);
};
#endif