#ifndef BTO_H
#define BTO_H

#include <iostream>
#include <mutex>
#include <vector>
#include <set>
#include <ctime>
#include <map>
#include <atomic>
#include <shared_mutex>

static std::atomic<int> counter{0};
static std::shared_mutex global_lock;

class data_item {
    public:
        std::mutex lock;
        int value = 0;
        int max_read = -1;
        int max_write = -1;
};

class transaction {
    public:
        int id;
        std::map<data_item*, int> local_write_set;
        char status;
        time_t start;
};

class BTO {
    public:
        transaction* begin_trans();
        int read(transaction*, data_item*);
        void write(transaction*, data_item*, int);
        char tryCommit(transaction*);
        FILE* f;
        BTO(FILE* f){
            this->f = f;
        }
        
};

#endif