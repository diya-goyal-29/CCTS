#ifndef FOCC_CTA_H
#define FOCC_CTA_H

#include "Data.h"
#include <iostream>
#include <atomic>
#include <stdio.h>

class FOCC_CTA{
public:
	int currTransNumber = 0;
	//std::set<Transaction*> LiveSet;
	
	Transaction *begin_trans();
	void read(Transaction *T, int index, DataItem *array, int local_data);
	void write(Transaction *T, int value, DataItem* index);
	char tryCommit(Transaction *T);
	
};


#endif // FOCC_CTA_H
