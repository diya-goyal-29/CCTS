#ifndef MVTO_UNBOUNDED_GARBAGE_COLLECTION_H
#define MVTO_UNBOUNDED_GARBAGE_COLLECTION_H

#include <iostream>
#include <mutex>
#include <vector>
#include <set>
#include <ctime>
#include <map>
#include <atomic>
#include <algorithm>
#include <shared_mutex>

static std::atomic<int> counter{0};
static std::shared_mutex global;
// static std::set<int> live_set;

class live_set{
    public:
        std::mutex lock;
        std::set<int> live;
};

class data_item{
    public:
        data_item* next = NULL;
        int value = 0;
        int id = -1;
        std::set<int> read_list = {};
};

class data_head{
    public:
        data_item* next;
        std::mutex lock;
        data_head() {
            this->next = new data_item();
        }
};

class transaction{
    public:
        int id;
        std::map <std::pair<data_head*,data_item*>, int> local_read_set;
        std::map <data_head*, int> local_write_set;
};

class MVTO_unbounded_gc {
    public:
        transaction* begin_trans();
        int read(transaction*, data_head *);
        void write(transaction*, data_head*, int);
        char tryCommit(transaction*);

};

#endif