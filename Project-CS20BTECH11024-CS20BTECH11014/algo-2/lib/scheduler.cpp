#include "scheduler.h"
#include "utils.h"
#include <atomic>
#include <cstdio>
#include <mutex>

Transaction *Scheduler::begin_trans(int ThId) {
  Transaction *T0 = new Transaction(++CurrTransNum, ThrMem->at(ThId)->P_Depend);
  return T0;
}

void Scheduler::read(Transaction *T, unsigned int X, int &L, int ThId) {
  if (T->Status == TransStatus::ABORT) {
    L = -1;
    return;
  }
  auto &LocalCopy = ThrMem->at(ThId)->LocalCopy;
  auto &P_Depend = ThrMem->at(ThId)->P_Depend;

  if (!LocalCopy.contains(X)) {
    DataItemInfo *XInfo = SharedMemory->at(X);
    XInfo->M.lock_shared();
    LocalCopy[X] = new DataItemInfo(*XInfo);
    // P_Depend = XInfo->Depend;
    XInfo->M.unlock_shared();
    T->ReadSet.insert(X);
    // T->T_Depend.insert({X, LocalCopy[X]->Depend[X]});
    T->T_Depend[X] = LocalCopy[X]->Depend[X];

    for (int Y : T->ReadSet) {
      if (T->T_Depend[Y] < LocalCopy[X]->Depend[Y]) {
        T->Status = TransStatus::ABORT;
        L = -1;
        return;
      }
    }

    for (unsigned Y = 0; Y < SharedMemory->size(); Y++) {
      if (!T->ReadSet.contains(Y)) {
        T->T_Depend[Y] = std::max(T->T_Depend[Y], LocalCopy[X]->Depend[Y]);
      }
    }
  }

  L = LocalCopy[X]->Val;
}

void Scheduler::write(Transaction *T, unsigned int X, const int L, int ThId) {
  if (T->Status == TransStatus::ABORT) {
    return;
  }
  auto &LocalCopy = ThrMem->at(ThId)->LocalCopy;

  if (!LocalCopy.contains(X)) {
    DataItemInfo *XInfo = SharedMemory->at(X);
    XInfo->M.lock_shared();
    LocalCopy[X] = new DataItemInfo(*XInfo);
    XInfo->M.unlock_shared();
  }
  LocalCopy[X]->Val = L;
  T->WriteSet.insert(X);
}

TransStatus Scheduler::tryCommit(Transaction *T, int ThId) {
  if (T->Status == TransStatus::ABORT) {
    garbageCollect(T, ThId);
    return TransStatus::ABORT;
  }
  bool ConsistencyCheck = true;
  std::set<int> UnionRW;

  for (int X : T->ReadSet)
    UnionRW.insert(X);
  for (int X : T->WriteSet)
    UnionRW.insert(X);

  for (int X : UnionRW)
    SharedMemory->at(X)->M.lock();

  for (int Z : T->ReadSet) {
    ConsistencyCheck &= (T->T_Depend[Z] == SharedMemory->at(Z)->Depend[Z]);
  }

  if (!T->ReadSet.empty()) {
    if (!ConsistencyCheck) {
      for (int X : UnionRW)
        SharedMemory->at(X)->M.unlock();
      garbageCollect(T, ThId);
      return TransStatus::ABORT;
    }
  }
  if (!T->WriteSet.empty()) {
    for (int X : T->WriteSet) {
      T->T_Depend[X] = SharedMemory->at(X)->Depend[X] + 1;
    }

    for (int X : T->WriteSet) {
      int Val = ThrMem->at(ThId)->LocalCopy[X]->Val;
      SharedMemory->at(X)->Val = Val;
      SharedMemory->at(X)->Depend = T->T_Depend;
      DEBUG(fprintf(LogFile, "From Transaction %d - SharedMemory[%d] = %d\n",
                    T->Id, X, Val));
    }
  }

  for (int X : UnionRW)
    SharedMemory->at(X)->M.unlock();
  ThrMem->at(ThId)->P_Depend.clear();
  ThrMem->at(ThId)->P_Depend = T->T_Depend;
  garbageCollect(T, ThId);
  return TransStatus::COMMIT;
}

void Scheduler::garbageCollect(Transaction *T, int ThId) {

  for (auto [_, XInfo] : ThrMem->at(ThId)->LocalCopy)
    delete XInfo;

  ThrMem->at(ThId)->LocalCopy.clear();
  delete T;
}