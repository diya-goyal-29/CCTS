#include "scheduler.h"
#include "utils.h"
#include <atomic>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iostream>
#include <random>
#include <sys/time.h>
#include <thread>
#include <unistd.h>

int N;
int M_DataItems;
int NumTrans;
int ConstVal;
int Lambda;
FILE *LogFile;
auto StartTime = std::chrono::system_clock::now();

std::random_device rd;
std::mt19937 gen(rd());

std::exponential_distribution<> ESdistrib;

// to be removed
std::atomic<double> CommitDelay;
std::atomic<double> AbortCnt;

template <typename Sch, typename Transaction, typename ThreadMemory>
void updtMem(Sch *Scheduler, int ThId) {
  TransStatus S = TransStatus::ABORT;
  int AbortCnt = 0;

  for (int CurrTrans = 0; CurrTrans < NumTrans; CurrTrans++) {
    AbortCnt = 0;
    auto CritStartTime = clock();

    do {

      Transaction *T = Scheduler->begin_trans(ThId);
      int TransId = T->Id;
      int RandIters = 1 + (rand() % M_DataItems);
      int LocalVal = -1;
      // std::cout << "Rand Iters = " << RandIters << "\n";
      for (int I = 0; I < RandIters; I++) {
        int RandInd = rand() % M_DataItems;
        int RandVal = rand() % ConstVal;

        // Scheduler->in(T, RandInd, LocalVal);
        Scheduler->read(T, RandInd, LocalVal, ThId);

        fprintf(
            LogFile,
            "Thread id %d Transaction %d reads from %d a value %d at time %d\n",
            ThId, TransId, RandInd, LocalVal, getTime(StartTime));

        LocalVal += RandVal;
        Scheduler->write(T, RandInd, LocalVal, ThId);

        fprintf(
            LogFile,
            "Thread id %d Transaction %d writes to %d a value %d at time %d\n",
            ThId, TransId, RandInd, LocalVal, getTime(StartTime));
        sleep(ESdistrib(gen));
      }
      S = Scheduler->tryCommit(T, ThId);

      if (S == TransStatus::COMMIT)
        fprintf(LogFile,
                "Transaction %d tryCommits with result COMMIT at time %d\n",
                TransId, getTime(StartTime));
      else
        fprintf(LogFile,
                "Transaction %d tryCommits with result ABORT at time %d\n",
                TransId, getTime(StartTime));
      AbortCnt++;

    } while (S != TransStatus::COMMIT);
    auto CritEndTime = clock();

    double CommitDelay = (double)(CritEndTime - CritStartTime) / CLOCKS_PER_SEC;
    CommitDelay *= pow(10, 3); // in milli seconds

    // record commitDelay & AbortCt;
    // AvgCommitDelay =
    //     (DoneTransCnt * AvgCommitDelay + CommitDelay) / (DoneTransCnt + 1);
    // AvgAbortCnt = (DoneTransCnt * AvgAbortCnt + AbortCnt) / (DoneTransCnt +
    // 1);
    // DoneTransCnt++;
    ::CommitDelay += CommitDelay;
    ::AbortCnt += AbortCnt;
  }
}

template <typename Sch, typename Transaction, typename DataItemInfo,
          typename ThreadMemory>
void init(const std::string &LogFileName) {
  std::vector<DataItemInfo *> SharedMemory(M_DataItems);
  for (int I = 0; I < M_DataItems; I++)
    SharedMemory[I] = new DataItemInfo;

  std::vector<ThreadMemory *> ThrMem(N);
  for (int I = 0; I < N; I++)
    ThrMem[I] = new ThreadMemory;

  CommitDelay = 0.0;
  AbortCnt = 0.0;

  LogFile = fopen(LogFileName.c_str(), "w");
  if (!LogFile) {
    std::cout << "LogFile is nullptr\n";
    exit(0);
  }

  Sch *Scheduler = new Sch(&SharedMemory, &ThrMem, LogFile);
  std::thread Threads[N];
  for (int I = 0; I < N; I++)
    Threads[I] =
        std::thread(updtMem<Sch, Transaction, ThreadMemory>, Scheduler, I);

  for (int I = 0; I < N; I++) {
    Threads[I].join();
  }

  delete Scheduler;

  for (int I = 0; I < M_DataItems; I++)
    delete SharedMemory[I];

  for (int I = 0; I < N; I++)
    delete ThrMem[I];

  fclose(LogFile);

  std::cout << CommitDelay / (NumTrans * N) << " " << AbortCnt / (NumTrans * N)
            << "\n";
}

int main(int argc, char *argv[]) {
  if (argc == 1) {
    std::cerr << "Please provide input file name\nExiting...\n";
    exit(0);
  }
  std::string InputFileName(argv[1]);

  std::ifstream InputFile(InputFileName);

  if (!InputFile) {
    std::cout << "InputFile is Nullptr\n";
    exit(0);
  }
  InputFile >> N >> M_DataItems >> NumTrans >> ConstVal >> Lambda;
  InputFile.close();

  // std::cout << N << " " << M_DataItems << " " << ConstVal << " " << Lambda
  //           << "\n";

  ESdistrib.param(std::exponential_distribution<double>::param_type(Lambda));

  init<Scheduler, Transaction, DataItemInfo, ThreadMemory>("Scheduler-log.txt");
  return 0;
}
