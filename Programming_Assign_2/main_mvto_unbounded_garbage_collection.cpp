#include "MVTO_unbounded_garbage_collection.h"
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
int numTrans;
int numData;
int constVal;
int lambda;
time_t avgTime = 0;
int abortCount = 0;
data_head* Memory;
std::string name = "MVTO_unbounded_garbage_collection_log_file.txt";

void uptMem(MVTO_unbounded_gc* MVTO_unboundedSch, FILE *file, int threadId);
int getSysTime();
auto StartTime = std::chrono::system_clock::now();

int main(int argc, char* argv[]) {
	
	std::string InputFileName(argv[1]);
	std::ifstream InputFile(InputFileName);
	if(!InputFile) 
		std::cout << "InputFile is nullptr\n";
	
	InputFile >> N >> numData >> numTrans >> constVal >> lambda;
	InputFile.close();
	
	MVTO_unbounded_gc MVTO_unboundedSch;
	Memory = new data_head[numData]();
	FILE *file = fopen(name.c_str(), "w");
	std::thread threads[N];
	for(int i = 0; i < N; i++) {
		threads[i] = std::thread(uptMem, &MVTO_unboundedSch, file, i);
	}
	for(int i = 0; i < N; i++) {
		threads[i].join();
	}
	std::cout << avgTime / (numTrans*N) << " " << abortCount / (numTrans*N);
}

int getSysTime() {
  auto CurrentTime = std::chrono::system_clock::now();
  auto TimeElapsed = CurrentTime - StartTime;
  return TimeElapsed.count();
}


void uptMem(MVTO_unbounded_gc* MVTO_unboundedSch, FILE *file, int threadId) {
	char status = 'a';
	int abortCnt = 0;
	
	for (int curTrans = 0; curTrans < numTrans; curTrans++) {
		abortCnt = 0;
		auto critStart = clock();
		do {
			transaction *T = MVTO_unboundedSch->begin_trans();
			int randIters = rand() % numData + 1;
			int localVal = 0;
			for(int i = 0; i < randIters; i++) {
				int randInd = rand() % numData;
				int randVal = rand() % constVal;
				localVal = MVTO_unboundedSch->read(T, &Memory[randInd]);
				fprintf(file, "Thread id %d Transaction %d reads from %d a value %d at time %d\n",threadId, T->id, randInd, localVal, getSysTime());
				localVal += randVal;
				MVTO_unboundedSch->write(T, &Memory[randInd], localVal);
				fprintf(file, "Thread id %d Transaction %d writes to %d a value %d at time %d\n", threadId, T->id, randInd, localVal, getSysTime());
			}

			std::default_random_engine generator;
			std::exponential_distribution<double> distribution1(1/lambda);
			double sleep_time = distribution1(generator);
			sleep(sleep_time);

            int tid = T->id;
			status = MVTO_unboundedSch->tryCommit(T);
			if(status == 'c') {
				fprintf(file, "Transaction %d tryCommits with result COMMIT at time %d\n", tid, getSysTime());
			} else {
				fprintf(file, "Transaction %d tryCommits with result ABORT at time %d\n", tid, getSysTime());
				abortCnt++;
			}
			
		} while(status != 'c');
		auto critEnd = clock();
		auto CommitDelay = ((critEnd - critStart) /(float) CLOCKS_PER_SEC) * pow(10,3);
		avgTime += CommitDelay;
		abortCount += abortCnt;
	}
}