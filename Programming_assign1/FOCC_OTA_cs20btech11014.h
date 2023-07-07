#ifndef FOCC_OTA_H
#define FOCC_OTA_H

#include "Data.h"
#include <iostream>

class FOCC_OTA{
public:
	int currTransNumber = 0;
	std::set<Transaction*> LiveSet;
	std::set<Transaction*>ConflictingSet;
	Transaction *begin_trans();
	void read(Transaction *T, int index, DataItem *array, int local_data);
	void write(Transaction *T, int value, DataItem* index);
	char tryCommit(Transaction *T);
	
};


#endif // FOCC_OTA_H
