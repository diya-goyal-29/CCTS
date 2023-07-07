#include "BOCC.h"
#include "Data.h"
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
DataItem *Memory;
std::string name = "BOCC_log_file.txt";

void uptMem(BOCC *boccSch, FILE *file, int threadId);
int getSysTime();
auto StartTime = std::chrono::system_clock::now();

int main(int argc, char *argv[])
{

	std::string InputFileName(argv[1]);
	std::ifstream InputFile(InputFileName);
	if (!InputFile)
		std::cout << "InputFile is nullptr\n";

	InputFile >> N >> numData >> numTrans >> constVal >> lambda;
	InputFile.close();

	BOCC boccSch;
	Memory = new DataItem[numData];
	FILE *file = fopen(name.c_str(), "w");
	std::thread threads[N];
	for (int i = 0; i < N; i++)
	{
		threads[i] = std::thread(uptMem, &boccSch, file, i);
	}
	for (int i = 0; i < N; i++)
	{
		threads[i].join();
	}
	std::cout << (float)avgTime / (numTrans) << " " << (float)abortCount / (numTrans);
}

int getSysTime()
{
	auto CurrentTime = std::chrono::system_clock::now();
	auto TimeElapsed = CurrentTime - StartTime;
	return TimeElapsed.count();
}

void uptMem(BOCC *boccSch, FILE *file, int threadId)
{
	char status = 'a';
	int abortCnt = 0;

	for (int curTrans = 0; curTrans < numTrans; curTrans++)
	{
		abortCnt = 0;
		auto critStart = clock();
		do
		{
			Transaction *T = boccSch->begin_trans();
			int randIters = rand() % numData + 1;
			int localVal = 0;
			for (int i = 0; i < randIters; i++)
			{
				int randInd = rand() % numData;
				int randVal = rand() % constVal;
				boccSch->read(T, randInd, Memory, localVal);
				fprintf(file, "Thread id %d Transaction %d reads from %d a value %d at time %d\n", threadId, T->id, randInd, localVal, getSysTime());
				localVal += randVal;
				boccSch->write(T, localVal, &Memory[randInd]);
				fprintf(file, "Thread id %d Transaction %d writes to %d a value %d at time %d\n", threadId, T->id, randInd, localVal, getSysTime());
			}
			status = boccSch->tryCommit(T);
			if (status == 'c')
			{
				fprintf(file, "Transaction %d tryCommits with result COMMIT at time %d\n", T->id, getSysTime());
			}
			else
			{
				fprintf(file, "Transaction %d tryCommits with result ABORT at time %d\n", T->id, getSysTime());
				abortCnt++;
			}

		} while (status != 'c');
		auto critEnd = clock();
		auto CommitDelay = ((critEnd - critStart) / CLOCKS_PER_SEC) * pow(10, 3);
		avgTime += CommitDelay;
		abortCount += abortCnt;
	}
}
