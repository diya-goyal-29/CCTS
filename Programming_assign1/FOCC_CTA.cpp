#include "FOCC_CTA.h"

Transaction *FOCC_CTA::begin_trans() {
	global_lock.lock();
	Transaction* T = new Transaction(currTransNumber);
	//LiveSet.insert(T);
	currTransNumber++;
	global_lock.unlock();
	return T;
}

void FOCC_CTA::read(Transaction* T, int index, DataItem *array, int local_data) {
	global_lock.lock_shared();
	//if(LiveSet.contains(T)){
		
		local_data = array[index].value;
		array[index].lock.lock_shared();
		T->ReadSet.insert(&array[index]);
		array[index].ReadList.insert(T);
		array[index].lock.unlock();
		
	//}
	global_lock.unlock();
}

void FOCC_CTA::write(Transaction* T, int value, DataItem* index) {
	global_lock.lock_shared();
	T->LocalWrite[index] = value;
	T->WriteSet.insert(index);
	global_lock.unlock();
}

char FOCC_CTA::tryCommit(Transaction *T) {
	global_lock.lock();
	std::set<Transaction*>ConflictingSet;
	
	int flag = 1;
	for(auto d : T->WriteSet){
		d->lock.lock();
		for(auto r : d->ReadList) {
			if(r->getStatus() == 'l' && T->id != r->id) {
				ConflictingSet.insert(r);
			}
		}
	}
    	
    	if(!ConflictingSet.empty()){
		flag = 0;
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

	    //LiveSet.erase(T);

	    if(flag){
		T->setStatus('c');
		for(auto d : T->WriteSet) {
		    d->value = T->LocalWrite[d];
		    d->lock.unlock();
		}
	    }
	    else{
		T->setStatus('a');
		for(auto d : T->WriteSet){
		    d->lock.unlock();
		}
		delete T;
		global_lock.unlock();
		return 'a';
	    }

	    delete T;
	    global_lock.unlock();
	    
	    return 'c';
			
}
