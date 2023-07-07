#include "MVTO_bounded.h"

transaction* MVTO_bounded::begin_trans(){
    transaction* t = new transaction();
    t->id = counter.fetch_add(1); //atomic counter
    t->status = 'l';
    return t;
}

int MVTO_bounded::read(transaction* t, data_head *x){
    int value = -1;
    if(t->status != 'a'){
        x->lock.lock();
        data_item* cur = x->next;
        data_item* previous = cur;
        int flag = 0;
        while(cur != NULL && cur->id < t->id){
            flag = 1;
            previous = cur;
            cur = cur->next;
        }
        if(flag){
            value = previous->value;
            t->local_read_set[{x , previous}] = value;
            previous->read_list.insert(t->id);
        } else {
            t->status = 'a';
        }
        x->lock.unlock();
    }
    return value;
}

void MVTO_bounded::write(transaction* t, data_head* x, int value){
    t->local_write_set[x] = value;
}

char MVTO_bounded::tryCommit(transaction* t) {
    if(t->status == 'a') {
        delete t;
        return 'a';
    }

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
            cur = cur->next;
        }
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

                if(it->first->count < k) {
                    it->first->count++;
                } else {
                    cur = it->first->next->next;
                    data_item *previous = it->first->next;
                    it->first->next = cur;
                    delete previous;
                }
                it->first->lock.unlock();
        }
        delete t;
        return 'c';
    }
    else {
        for(std::map<std::pair<data_head*, data_item*>, int>::iterator it = t->local_read_set.begin(); it != t->local_read_set.end(); ++it) {
            if(t->local_write_set.find(it->first.first) == t->local_write_set.end()){
                it->first.first->lock.lock();
            }
            it->first.second->read_list.erase(t->id);
            if(t->local_write_set.find(it->first.first) == t->local_write_set.end()){
                it->first.first->lock.unlock();
            }
        }
        for(std::map<data_head*, int>::iterator it = t->local_write_set.begin(); it != t->local_write_set.end(); ++it){
            it->first->lock.unlock();
        }
        delete t;
        return 'a';
    }

}