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

// Memory class that manages a scope for data and threads
class Memory {
private:
    std::shared_ptr<Memory> parent;
    std::unordered_map<std::string, std::shared_ptr<Data>> data;
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
    std::shared_ptr<Data> get(const std::string& item);
    std::shared_ptr<Data> get(const std::string& item, bool allowMutable);
    std::shared_ptr<Data> getOrNull(const std::string& item, bool allowMutable);
    void set(const std::string& item, std::shared_ptr<Data> value);

    // Methods to manage inheritance and synchronization with other Memory objects
    void pull(std::shared_ptr<Memory> other);
    void replaceMissing(std::shared_ptr<Memory> other);
    void detach();
    void detach(std::shared_ptr<Memory> par);
};

#endif // MEMORY_H
