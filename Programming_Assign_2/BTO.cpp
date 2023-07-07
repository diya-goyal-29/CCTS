#include "BTO.h"

transaction* BTO::begin_trans(){
    transaction* t = new transaction();
    t->id = counter.fetch_add(1); //atomic counter
    t->status = 'l';
    t->start = clock();
    return t;
}

int BTO::read(transaction* t, data_item* x) {
    global_lock.lock_shared();
    int value = -1;
    if(t->status != 'a'){
        x->lock.lock();
        if(x->max_write < t->id){
            value = x->value;
            // x->max_read = clock();
            x->max_read = t->id;
		    // fprintf(BTO::f, "read trasanction number %d, memory address %d, value %d\n", t->id, x->max_write, x->value);
        } else{
            t->status = 'a';
        }
        x->lock.unlock();
    }
    global_lock.unlock_shared();
    return value;
}

void BTO::write(transaction* t, data_item* x, int value){
    t->local_write_set[x] = value;
}

char BTO::tryCommit(transaction* t) {
    global_lock.lock();
    if(t->status == 'a') {
        delete t;
        global_lock.unlock();
        return 'a';
    }
    int flag = 1;
    for(std::map<data_item*, int>::iterator it = t->local_write_set.begin(); it != t->local_write_set.end(); ++it){
        auto cur = it->first;
        cur->lock.lock();
        if(cur->max_read > t->id || cur->max_write > t->id) 
            flag = 0;
        
    }
    if(flag) {
        for(std::map<data_item*, int>::iterator it = t->local_write_set.begin(); it != t->local_write_set.end(); ++it) {
            auto cur = it->first;
            int val = it->second;
            cur->value = val;
            // std::cout<< "tid " << t->id<<" value " <<val << "\n";
		    // fprintf(BTO::f, "trasanction number %d, memory address %d, value %d\n", t->id, cur->max_write, cur->value);
            cur->max_write = t->id;
            cur->lock.unlock();
        }
        delete t;
        global_lock.unlock();
        return 'c';
    } else {
        for(std::map<data_item*, int>::iterator it = t->local_write_set.begin(); it != t->local_write_set.end(); ++it) {
            it->first->lock.unlock();
        }
        delete t;
        global_lock.unlock();
        return 'a';
    }
}