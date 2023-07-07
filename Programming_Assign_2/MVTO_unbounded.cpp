#include "MVTO_unbounded.h"

transaction* MVTO_unbounded::begin_trans() {
    transaction* t = new transaction();
    t->id = counter.fetch_add(1); //atomic counter
    // MVTO_unbounded::live_set.insert(t->id);
    // live_set.insert(t->id);
    return t;
}

int MVTO_unbounded::read(transaction* t, data_head* x) {
    global.lock_shared();
    x->lock.lock();
    data_item* cur = x->next;
    data_item* previous = cur;
    while(cur != NULL && cur->id < t->id){
        previous = cur;
        cur = cur->next;
    }
    // std::cout << previous->id << " " <<previous->value << "\n";
    int value = previous->value;
    int id_ = t->id;
    previous->read_list.insert(id_);
    t->local_read_set[std::make_pair(x, previous)] = value;
    x->lock.unlock();
    global.unlock_shared();
    return value;
}

void MVTO_unbounded::write(transaction* t, data_head* x, int value){
    t->local_write_set[x] = value;
}

char MVTO_unbounded::tryCommit(transaction* t){
    global.lock();
    int flag = 1;
    for(std::map<data_head*, int>::iterator it = t->local_write_set.begin(); it != t->local_write_set.end(); ++it) {
        it->first->lock.lock();
        data_item* cur = it->first->next;
        while(cur != NULL && cur->id < t->id) {
            for(auto i : cur->read_list){
                if(i > t->id){
                    flag = 0;
                    break;
                }
            }
            if(!flag)
                break;
            cur = cur->next;
        }
        if(!flag)
            break;
    }
    if(flag){
        for(std::map<data_head*, int>::iterator it = t->local_write_set.begin(); it != t->local_write_set.end(); ++it){
            data_item* cur = it->first->next;
            data_item* prev = cur;
            while(cur->next != NULL && cur->id < t->id){
                prev = cur;
                cur = cur->next;
            }
            data_item* temp = prev->next;
            data_item *data = new data_item();
            prev->next = data;
            prev->next->next = temp;
            prev->next->value = it->second;
            prev->next->id = t->id;
            it->first->lock.unlock();
        }
        // live_set.erase(t->id);
        global.unlock();
        delete t;
        return 'c';
    }
    else {
        for(std::map<std::pair<data_head*, data_item*>, int>::iterator it = t->local_read_set.begin(); it != t->local_read_set.end(); ++it) {
            if(t->local_write_set.find(it->first.first) == t->local_write_set.end()){
                it->first.first->lock.lock();
            }
            int id_ = t->id;
            it->first.second->read_list.erase(id_);
            if(t->local_write_set.find(it->first.first) == t->local_write_set.end()){
                it->first.first->lock.unlock();
            }
        }
        for(std::map<data_head*, int>::iterator it = t->local_write_set.begin(); it != t->local_write_set.end(); ++it){
            it->first->lock.unlock();
        }
        // live_set.erase(t->id);
        global.unlock();
        delete t;
        return 'a';
    }
}
