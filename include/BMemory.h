// Memory.h
#ifndef BMEMORY_H
#define BMEMORY_H

#include <memory>
#include <unordered_map>
#include <string>
#include <stack>
#include <vector>
#include <iostream>
#include <atomic>
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
    int atomicId;
    int callId;
    VariableManager() {
        //lastId = getId("LAST");
        thisId = getId("this");
        argsId = getId("args");
        noneId = getId("#");
        callId = getId("\\call");
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
    BMemory* parent;
    tsl::hopscotch_map<int, Data*> *data;
    pthread_mutex_t memoryLock;
    tsl::hopscotch_set<int> finals;
    Data* fastLastAccess;
    int fastLastAccessId;

public:
    std::atomic<int> countDependencies;
    tsl::hopscotch_set<Future*> attached_threads;
    bool allowMutables;
    bool isOrDerivedFrom(BMemory* memory) const;

    // Constructors and destructor=
    explicit BMemory(BMemory* par, int expectedAssignments);
    bool release(Data* preserve);
    void release();
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
    void unsafeSet(int item, Data* value, Data* prev) ;
    int size() const;
    void removeWithoutDelete(int item);
    void setFinal(int item) ;
    bool isFinal(int item) const;

    // Methods to manage inheritance and synchronization with other Memory objects
    void pull(BMemory*  other);
    void replaceMissing(BMemory*  other);
    void detach();
    void detach(BMemory*  par);

    static void verify_noleaks();
};

#endif // MEMORY_H
