// Memory.h
#ifndef BMEMORY_H
#define BMEMORY_H

#include <memory>
#include <unordered_map>
#include <string>
#include <stack>
#include <vector>
#include <iostream>
#include <pthread.h>
#include "data/Data.h"
#include "data/Future.h"
#include "tsl/hopscotch_map.h"
#include "tsl/hopscotch_set.h"


class VariableManager {
private:
    tsl::hopscotch_map<std::string, int> registeredSymbols;
    tsl::hopscotch_map<int, std::string> registeredIds;
public:
    //int lastId;
    int thisId;
    int argsId;
    int noneId;
    VariableManager() {
        //lastId = getId("LAST");
        thisId = getId("this");
        argsId = getId("args");
        noneId = getId("#");
    }
    int getId(const std::string& symbol) {
        if(registeredSymbols.find(symbol)==registeredSymbols.end()) {
            int id = registeredSymbols.size();
            registeredSymbols[symbol] = id;
            registeredIds[id] = symbol;
        }
        return registeredSymbols[symbol];
    }
    const std::string getSymbol(int id) {
        return registeredIds[id];
    }
};

class Command;

// Memory class that manages a scope for data and threads
class BMemory {
private:
    std::shared_ptr<BMemory> parent;
    tsl::hopscotch_map<int, Data*> *data;
    bool allowMutables;
    pthread_mutex_t memoryLock;
    tsl::hopscotch_set<int> finals;

public:
    tsl::hopscotch_set<Future*> attached_threads;
    std::stack<tsl::hopscotch_map<int, Data*>*> *mapPool;
    bool isOrDerivedFrom(const std::shared_ptr<BMemory>& memory) const;

    // Constructors and destructor=
    explicit BMemory(const std::shared_ptr<BMemory>& par, int expectedAssignments);
    void release();
    void releaseNonFinals();
    ~BMemory();

    // Lock and unlock methods for thread safety
    void lock();
    void unlock();

    // Methods to get and set data
    bool contains(int item) const;
    Data* get(int item) ;
    Data* get(int item, bool allowMutable);
    Data* getOrNull(int item, bool allowMutable);
    Data* getOrNullShallow(int item);
    void set(int item, Data* value) ;
    void unsafeSet(int item, Data* value, Data* prev) ;
    int size() const;
    void removeWithoutDelete(int item);
    void setFinal(int item) ;
    bool isFinal(int item) const;

    // Methods to manage inheritance and synchronization with other Memory objects
    void pull(const std::shared_ptr<BMemory>& other);
    void replaceMissing(const std::shared_ptr<BMemory>& other);
    void detach();
    void detach(const std::shared_ptr<BMemory>& par);
};

#endif // MEMORY_H
