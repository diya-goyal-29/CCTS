#ifndef DATA_H
#define DATA_H

#include <mutex>
#include <vector>
#include <set>
#include <ctime>
#include <map>
#include <shared_mutex>

static std::shared_mutex global_lock;
class DataItem;

class Transaction {
private:
	char Status = 'l';
	
public:
	const int id;
	int reference_count;
	std::set<DataItem*> ReadSet;
	std::set<DataItem*> WriteSet;
	std::set<Transaction*> locLiveSet;
	std::map<DataItem*,int> LocalWrite;
	time_t Start;
	time_t End;
	
	void setStatus(char S){
		Status = S;
	}
	char getStatus(){
		char s = this->Status;
		return s;
	}
	
	Transaction(int id, int count=0) : id(id) {
		reference_count = count;
		Start = clock();
		
	}
	
};

class DataItem {
	
public:
	int value = 0;
	std::shared_mutex lock;
	std::set<Transaction *> ReadList;
	std::set<Transaction *> WriteList;
};

#endif //DATA_H
