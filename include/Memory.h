// Memory.h
#ifndef MEMORY_H
#define MEMORY_H

#include <memory>
#include <unordered_map>
#include <string>
#include <vector>
#include <iostream>
#include <pthread.h>
#include "Data.h"
#include "Future.h"
#include "tsl/hopscotch_map.h"


class VariableManager {
private:
    tsl::hopscotch_map<std::string, int> registeredSymbols;
    tsl::hopscotch_map<int, std::string> registeredIds;
public:
    int argsId;
    int thisId;
    int lastId;
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

// Memory class that manages a scope for data and threads
class Memory {
private:
    std::shared_ptr<Memory> parent;
    tsl::hopscotch_map<int, std::shared_ptr<Data>> data;
    tsl::hopscotch_map<int, std::shared_ptr<Data>> locals;
    bool allowMutables;
    pthread_mutex_t memoryLock;

public:
    std::vector<std::shared_ptr<Future>> attached_threads;

    // Constructors and destructor
    Memory();
    explicit Memory(std::shared_ptr<Memory> par);
    ~Memory();

    // Lock and unlock methods for thread safety
    void lock();
    void unlock();

    // Methods to get and set data
    std::shared_ptr<Data> get(int item);
    std::shared_ptr<Data> getLocal(int item);
    std::shared_ptr<Data> get(int item, bool allowMutable);
    std::shared_ptr<Data> getOrNull(int item, bool allowMutable);
    std::shared_ptr<Data> getOrNullShallow(int item);
    void set(int item, const std::shared_ptr<Data>& value);
    void setLocal(int item, const std::shared_ptr<Data>& value);

    // Methods to manage inheritance and synchronization with other Memory objects
    void pull(std::shared_ptr<Memory> other);
    void replaceMissing(std::shared_ptr<Memory> other);
    void detach();
    void detach(std::shared_ptr<Memory> par);
};

#endif // MEMORY_H
