#include "FOCC_OTA.h"

Transaction *FOCC_OTA::begin_trans() {
	global_lock.lock();
	Transaction* T = new Transaction(currTransNumber, LiveSet.size());
	LiveSet.insert(T);
	currTransNumber++;
	global_lock.unlock();
	return T;
}

void FOCC_OTA::read(Transaction* T, int index, DataItem *array, int local_data) {
	if(LiveSet.contains(T)){
		global_lock.lock_shared();
		local_data = array[index].value;
		array[index].lock.lock_shared();
		T->ReadSet.insert(&array[index]);
		array[index].ReadList.insert(T);
		array[index].lock.unlock();
		global_lock.unlock();
	}
}

void FOCC_OTA::write(Transaction* T, int value, DataItem* index) {
	T->LocalWrite[index] = value;
	T->WriteSet.insert(index);
}

char FOCC_OTA::tryCommit(Transaction *T) {
	global_lock.lock();
	std::set<Transaction*>ConflictingSet;
	if(T->getStatus() == 'a') {
		for(auto e : T->ReadSet) {
			e->lock.lock();
			e->ReadList.erase(T);
			e->lock.unlock();
		}
		delete T;
		global_lock.unlock();
		return 'a';
	}
	for (auto d : T->WriteSet){
		d->lock.lock();
		for (auto t : d->ReadList) {
			if(t->getStatus() == 'l') {
				ConflictingSet.insert(t);
			}
		}
	}
	
	if(ConflictingSet.contains(T)){
		ConflictingSet.erase(T);
        }
        
        for (auto e : T->ReadSet) {
		if(!T->WriteSet.contains(e)){
			e->lock.lock();
		}
		e->ReadList.erase(T);
		if(!T->WriteSet.contains(e)){
			e->lock.unlock();
		}
	} 
	
	for ( auto t : ConflictingSet) {
		t->setStatus('a');
		LiveSet.erase(t);
	}
	
	T->setStatus('c');
	for (auto d : T->WriteSet){
		d->value = T->LocalWrite[d];
		d->lock.unlock();
	}
	
	LiveSet.erase(T);
	delete T;
	global_lock.unlock();
	
	return 'c';
}
