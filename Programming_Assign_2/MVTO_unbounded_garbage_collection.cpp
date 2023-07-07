#include"MVTO_unbounded_garbage_collection.h"

live_set *ls = new live_set();

transaction* MVTO_unbounded_gc::begin_trans() {
    ls->lock.lock();
    transaction* t = new transaction();
    t->id = counter.fetch_add(1); //atomic counter
    ls->live.insert(t->id);
    ls->lock.unlock();
    return t;
}

int MVTO_unbounded_gc::read(transaction* t, data_head* x) {
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
    previous->read_list.insert(t->id);
    t->local_read_set[std::make_pair(x, previous)] = value;
    x->lock.unlock();
    global.unlock_shared();
    return value;
}

void MVTO_unbounded_gc::write(transaction* t, data_head* x, int value){
    t->local_write_set[x] = value;
}

char MVTO_unbounded_gc::tryCommit(transaction* t){
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
            data_item* previous = cur;
            while(cur->next != NULL && cur->id < t->id){
                previous = cur;
                cur = cur->next;
            }
            data_item *data = new data_item();
            data_item* temp = previous->next;
            previous->next = data;
            previous->next->next = temp;
            previous->next->value = it->second;
            previous->next->id = t->id;
        }

        ls->lock.lock();
        int minima = *ls->live.begin();

        //garbage collection
        
        for(std::map<data_head*, int>::iterator it = t->local_write_set.begin(); it != t->local_write_set.end(); ++it){
            data_item* cur = it->first->next;
            data_item* previous = cur;
            while(cur != NULL){ 
                if(cur->id < minima){
                    cur = cur->next;
                    if(cur->id < minima){
                        it->first->next = cur;
                        delete previous;
                        previous = cur;
                    }
                } else{
                    break;
                }
            }
            it->first->lock.unlock();
        }
        
        ls->live.erase(t->id);
        ls->lock.unlock();
        global.unlock();
        delete t;
        return 'c';
    }
    else {
        for(std::map<std::pair<data_head*, data_item*>, int>::iterator it = t->local_read_set.begin(); it != t->local_read_set.end(); ++it) {
            if(t->local_write_set.find(it->first.first) == t->local_write_set.end()){
                it->first.first->lock.lock();
            }
            it->first.second->read_list.erase(it->first.second->read_list.find(t->id));
            if(t->local_write_set.find(it->first.first) == t->local_write_set.end()){
                it->first.first->lock.unlock();
            }
        }
        for(std::map<data_head*, int>::iterator it = t->local_write_set.begin(); it != t->local_write_set.end(); ++it){
            it->first->lock.unlock();
        }
        ls->lock.lock();
        ls->live.erase(t->id);
        ls->lock.unlock();
        global.unlock();
        delete t;
        return 'a';
    }
}