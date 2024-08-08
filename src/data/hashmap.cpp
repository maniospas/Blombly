#include "data/BHashMap.h"
#include "data/Integer.h"
#include "data/Boolean.h"
#include "data/BString.h"
#include "data/BFloat.h"
#include "common.h"
#include <iostream>
#include <unordered_map>
#include <memory>
#include <pthread.h>
#include "data/Iterator.h"
#include "tsl/hopscotch_map.h"
#include "tsl/hopscotch_set.h"


HashMapContents::HashMapContents() : lockable(0) {
    if (pthread_mutex_init(&memoryLock, nullptr) != 0) 
        bberror("Failed to create a mutex for hash map read/write");
}

HashMapContents::~HashMapContents() {
    for (const auto& pair : contents) {
        if (pair.second && pair.second->isDestroyable) 
            delete pair.second;
    }
}

void HashMapContents::lock() {
    if (lockable)
        pthread_mutex_lock(&memoryLock);
}

void HashMapContents::unlock() {
    if (lockable)
        pthread_mutex_unlock(&memoryLock);
}

void HashMapContents::unsafeUnlock() {
    pthread_mutex_unlock(&memoryLock);
}

BHashMap::BHashMap() : contents(std::make_shared<HashMapContents>()) {}

BHashMap::BHashMap(const std::shared_ptr<HashMapContents>& cont) : contents(cont) {}

BHashMap::~BHashMap() {
    contents->lock();
    bool shouldUnlock = contents->lockable;
    contents->lockable -= 1;
    if (shouldUnlock)
        contents->unsafeUnlock();
}

int BHashMap::getType() const {
    return MAP;
}

std::string BHashMap::toString() const {
    contents->lock();
    std::string result = "[";
    for (const auto& pair : contents->contents) {
        if (result.size() > 1) 
            result += ", ";
        result += pair.second->toString();
    }
    contents->unlock();
    return result + "]";
}

Data* BHashMap::shallowCopy() const {
    contents->lock();
    Data* ret = new BHashMap(contents);
    bool shouldUnlock = contents->lockable;
    contents->lockable += 1;
    if (shouldUnlock)
        contents->unlock();
    return ret;
}

void BHashMap::put(Data* from, Data* to) {
    bbassert(from, "Missing key value");
    contents->lock();
    size_t key = from->toHash();
    Data* prev = contents->contents[key];
    if (prev && prev->isDestroyable) {
        delete prev;
    }
    contents->contents[key] = to ? to->shallowCopyIfNeeded() : to;
    contents->unlock();
}

Data* BHashMap::implement(const OperationType operation, BuiltinArgs* args) {
    if (args->size == 1) {
        switch (operation) {
            case TOCOPY: return shallowCopyIfNeeded();
            case LEN: return new Integer(contents->contents.size());
            case TOITER: return new Iterator(std::make_shared<IteratorContents>(this));
            // Implement other operations as needed
        }
        throw Unimplemented();
    }
    if (operation == AT && args->size == 2) {
        contents->lock();
        size_t key = args->arg1->toHash();
        auto it = contents->contents.find(key);
        if (it == contents->contents.end()) {
            contents->unlock();
            return nullptr;
        }
        Data* ret = it->second;
        if (ret) {
            auto type = ret->getType();
            Data* res = args->preallocResult;
            if (res && res->getType() == type) {
                if (type == INT) {
                    static_cast<Integer*>(args->preallocResult)->value = static_cast<Integer*>(ret)->value;
                    ret = res;
                } else if (type == FLOAT) {
                    static_cast<BFloat*>(args->preallocResult)->value = static_cast<BFloat*>(ret)->value;
                    ret = res;
                } else if (type == BOOL) {
                    static_cast<Boolean*>(args->preallocResult)->value = static_cast<Boolean*>(ret)->value;
                    ret = res;
                } else if (type == STRING) {
                    static_cast<BString*>(args->preallocResult)->value = static_cast<BString*>(ret)->value;
                    ret = res;
                }
            } else {
                ret = ret->shallowCopyIfNeeded();
            }
        }
        contents->unlock();
        return ret;
    }

    if (operation == PUT && args->size == 3) {
        contents->lock();
        size_t key = args->arg1->toHash();
        Data* prev = contents->contents[key];
        if (prev && prev->isDestroyable) {
            delete prev;
        }
        contents->contents[key] = args->arg2?args->arg2->shallowCopyIfNeeded():nullptr;
        contents->unlock();
        return nullptr;
    }

    throw Unimplemented();
}
