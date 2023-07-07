#ifndef BOCC_H
#define BOCC_H

#include "Data.h"
#include <iostream>

class BOCC{
public:
	int currTransNumber = 0;
	std::set<Transaction*> LiveSet;
	std::set<Transaction*>CommittedList;
	Transaction *begin_trans();
	void read(Transaction *T, int index, DataItem *array, int local_data);
	void write(Transaction *T, int value, DataItem* index);
	char tryCommit(Transaction *T);
	void Garbage_Collection(Transaction *T);
	
};


#endif // BOCC_H
