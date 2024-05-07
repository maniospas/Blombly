// Memory.cpp
#include "Memory.h"
#include "common.h"
#include "Future.h"

extern VariableManager variableManager;

// Default constructor
Memory::Memory() : parent(nullptr), allowMutables(true) {
    if (pthread_mutex_init(&memoryLock, nullptr) != 0) {
        std::cerr << "Failed to create a mutex for memory read/write" << std::endl;
        exit(1);
    }
}

// Constructor with parent memory
Memory::Memory(std::shared_ptr<Memory> par) : parent(std::move(par)), allowMutables(true) {
    if (pthread_mutex_init(&memoryLock, nullptr) != 0) {
        std::cerr << "Failed to create a mutex for struct read/write" << std::endl;
        exit(1);
    }
}

// Destructor to ensure that all threads are finished
Memory::~Memory() {
    for (std::shared_ptr<Future> thread : attached_threads) {
        thread->getResult();
    }
    pthread_mutex_destroy(&memoryLock);
}

// Lock the memory for thread-safe access
void Memory::lock() {
    if (parent) {
        parent->lock();
    }
    pthread_mutex_lock(&memoryLock);
}

// Unlock the memory
void Memory::unlock() {
    pthread_mutex_unlock(&memoryLock);
    if (parent) {
        parent->unlock();
    }
}

// Get a data item with mutability check
std::shared_ptr<Data> Memory::get(int item) {
    return get(item, true);
}

// Get a data item, optionally allowing mutable values
std::shared_ptr<Data> Memory::get(int item, bool allowMutable) {
    //if (item == "#") {
    //    return nullptr;
    //}

    //lock();
    std::shared_ptr<Data> ret = data[item];
    //unlock();

    // Handle future values
    if (ret && ret->getType() == FUTURE) {
        ret = std::static_pointer_cast<Future>(ret)->getResult();
        //lock();
        data[item] = ret;
        //unlock();
    }

    // Handle mutability restrictions
    if (ret && !allowMutable && ret->isMutable) {
        std::cerr << "Mutable symbol cannot be accessed from a nested block: " + variableManager.getSymbol(item) << std::endl;
        exit(1);
        return nullptr;
    }

    // If not found locally, check parent memory
    if (!ret && parent) {
        ret = parent->get(item, allowMutables);
    }

    // Missing value error
    if (!ret) {
        std::cerr << "Missing value: " + variableManager.getSymbol(item) << std::endl;
        exit(1);
    }

    return ret;
}


std::shared_ptr<Data> Memory::getOrNullShallow(int item) {
    return data[item];
}

// Get a data item or return nullptr if not found
std::shared_ptr<Data> Memory::getOrNull(int item, bool allowMutable) {
    //if (item == "#") {
    //    return nullptr;
    //}

    std::shared_ptr<Data> ret;
    //lock();
    ret = data[item];
    //unlock();

    // Handle future values
    if (ret && ret->getType() == FUTURE) {
        ret = std::static_pointer_cast<Future>(ret)->getResult();
        //lock();
        data[item] = ret;
        //unlock();
    }

    // Handle mutability restrictions
    if (ret && !allowMutable && ret->isMutable) {
        std::cerr << "Mutable symbol cannot be accessed from a nested block: " + item << std::endl;
        exit(1);
        return nullptr;
    }

    // If not found locally, check parent memory
    if (!ret && parent) {
        ret = parent->getOrNull(item, allowMutables);
    }

    return ret;
}

// Set a data item, ensuring mutability rules are followed
void Memory::set(int item, const std::shared_ptr<Data>& value) {
    //if (item == "#") {
    //    return;
    //}

    //lock();
    if (data[item] != nullptr && !data[item]->isMutable) {
        bool couldBeShallowCopy = data[item]->couldBeShallowCopy(value);
        //unlock();
        if (!couldBeShallowCopy) {
            std::cerr << "Cannot overwrite final value: " + variableManager.getSymbol(item) << std::endl;
            exit(1);
        }
        return;
    }
    data[item] = value;
    //unlock();
}

// Pull data from another Memory object
void Memory::pull(std::shared_ptr<Memory> other) {
    for (auto& it : other->data) {
        set(it.first, it.second);
    }
}

// Replace missing values with those from another Memory object
void Memory::replaceMissing(std::shared_ptr<Memory> other) {
    for (auto& it : other->data) {
        if (!data[it.first]) {
            set(it.first, it.second);
        }
    }
}

// Detach this memory from its parent
void Memory::detach() {
    allowMutables = false;
    parent = nullptr;
}

// Detach this memory from its parent but retain reference
void Memory::detach(std::shared_ptr<Memory> par) {
    allowMutables = false;
    parent = std::move(par);
}
