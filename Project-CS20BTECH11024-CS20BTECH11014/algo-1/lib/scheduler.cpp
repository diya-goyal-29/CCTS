#include "scheduler.h"
#include "utils.h"
#include <atomic>
#include <cstdio>
#include <mutex>

Transaction *Scheduler::begin_trans() {
  Transaction *T0 = new Transaction(++CurrTransNum);
  return T0;
}

int Scheduler::invoke(Transaction *T, Operation Op, unsigned int X, int L) {
  if (T->Status == TransStatus::ABORT) {
    return -1;
  }
  DataItemInfo *XInfo = SharedMemory->at(X);
  int State, Ts;
  if (T->WriteSet.contains(X)) {
    State = T->LocalCopy[X].first;
    Ts = T->LocalCopy[X].second;
  } else {
    if (Op != Operation::READ) {
      bool LockStatus = XInfo->M.try_lock(T->Id);
      // fprintf(LogFile, "T.id = %d locking X = %d : Lock Status = %d\n",
      // T->Id, X, LockStatus);

      if (!LockStatus) {
        T->Status = TransStatus::ABORT;
        return -1;
      } else {
        int p = XInfo->M.m_tid;
        // fprintf(LogFile, "T.id = %d : m_var = %d\n", T->Id, p);
        T->LockedSet.insert(X);
      }
    } else if (XInfo->M.isLocked()) { // how to check M.isLocked()
      T->Status = TransStatus::ABORT;
      return -1;
    }

    if (T->ReadSet.find(X) != T->ReadSet.end()) {
      State = T->LocalCopy[X].first;
      Ts = T->LocalCopy[X].second;
    } else {
      State = XInfo->State;
      Ts = XInfo->Ts;
    }
  }
  int NewState = State;
  if (Op == Operation::WRITE)
    NewState = L;

  T->LocalCopy[X] = {NewState, Ts};

  (Op == Operation::READ) ? (T->ReadSet.insert(X)) : (T->WriteSet.insert(X));
  return NewState;
}

TransStatus Scheduler::tryCommit(Transaction *T) {
  if (T->Status == TransStatus::ABORT) {
    reset(T);
    return TransStatus::ABORT;
  }
  // fprintf(LogFile, "T.id = %d : Entered tryC\n", T->Id);
  int State, Ts;
  int CurState, CurTs;
  for (int X : T->ReadSet) {
    State = T->LocalCopy[X].first;
    Ts = T->LocalCopy[X].second;

    DataItemInfo *XInfo = SharedMemory->at(X);
    CurState = XInfo->State;
    CurTs = XInfo->Ts;

    if (!(XInfo->M.has_lock(T->Id)) || (CurTs > Ts)) {
      reset(T);
      return TransStatus::ABORT;
    }
  }

  if (!T->WriteSet.empty()) {
    Wts++;
    for (auto X : T->WriteSet) {
      State = T->LocalCopy[X].first;
      Ts = T->LocalCopy[X].second;
      SharedMemory->at(X)->State = State;
      SharedMemory->at(X)->Ts = Wts;
      DEBUG(fprintf(LogFile, "From Transaction %d - SharedMemory[%d] = %d\n",
                    T->Id, X, State));
    }
  }
  reset(T);
  return TransStatus::COMMIT;
}

void Scheduler::reset(Transaction *T) {
  for (int X : T->LockedSet) {
    SharedMemory->at(X)->M.unlock();
  }
  delete T;
}