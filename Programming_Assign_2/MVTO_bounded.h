#ifndef MVTO_BOUNDED_H
#define MVTO_BOUNDED_H

#include <iostream>
#include <mutex>
#include <vector>
#include <set>
#include <ctime>
#include <map>
#include <atomic>
#include <shared_mutex>

// int k = 5; // number of versions that can be made
static std::atomic<int> counter{0};

class data_item{
    public:
        data_item* next = NULL;
        int value = 0;
        int id = -1;
        std::set<int> read_list;
};

class data_head{
    public:
        data_item* next;
        std::mutex lock;
        int count = 0;
        data_head() {
            this->next = new data_item();
        }
};

class transaction{
    public:    
        int id;
        std::map <std::pair<data_head*,data_item*>, int> local_read_set;
        std::map <data_head*, int> local_write_set;
        char status;
};

class MVTO_bounded{
    public:
        int k;
        transaction* begin_trans();
        int read(transaction*, data_head *);
        void write(transaction*, data_head*, int);
        char tryCommit(transaction*);
        MVTO_bounded(int k) {
            this->k = k;
        }
};

#endif