// BString.cpp
#include "data/BString.h"
#include "data/Boolean.h"
#include "data/Integer.h"
#include "data/BFloat.h"
#include "data/BFile.h"
#include "data/Iterator.h"
#include "common.h"

#include "data/BFile.h"
#include <iostream>
#include <fstream>
#include <pthread.h>

// RawFile constructor
RawFile::RawFile(const std::string& path_) : path(path_), size(0), lockable(0) {
    if (pthread_mutex_init(&memoryLock, nullptr) != 0) {
        bberror("Failed to create a mutex for file read/write");
    }
    std::ifstream file(path);
    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line)) 
            contents.push_back(line);
        file.close();
    } else {
        bberror("Failed to open file: " + path);
    }
    size = contents.size();
}

// Lock the file
void RawFile::lock() {
    if (lockable)
        pthread_mutex_lock(&memoryLock);
}

// Unlock the file
void RawFile::unlock() {
    if (lockable)
        pthread_mutex_unlock(&memoryLock);
}

// Unsafe unlock for the file
void RawFile::unsafeUnlock() {
    pthread_mutex_unlock(&memoryLock);
}



#include "data/BFile.h"
#include "data/Integer.h"
#include "data/BFloat.h"
#include "data/BString.h"
#include "common.h"
#include <iostream>

// BFile constructor
BFile::BFile(const std::string& path_) : rawFile(std::make_shared<RawFile>(path_)) {}

// BFile constructor with shared RawFile
BFile::BFile(const std::shared_ptr<RawFile>& rawFile_) : rawFile(rawFile_) {}

// Get the type of the object
int BFile::getType() const {
    return FILETYPE;
}

// Convert the file content to a string
std::string BFile::toString() const {
    rawFile->lock();
    std::string result = "file@" + rawFile->path+":";
    for (std::size_t i = 0; i < std::min(static_cast<std::size_t>(5), rawFile->contents.size()); ++i) {
        result += "\n";
        result += rawFile->contents[i];
    }
    if (rawFile->contents.size() > 5) 
        result += "\n...";
    rawFile->unlock();
    return result;
}

// Create a shallow copy of the file
Data* BFile::shallowCopy() const {
    rawFile->lock();
    BFile* ret = new BFile(rawFile);
    bool shouldUnlock = rawFile->lockable;
    rawFile->lockable += 1;
    if (shouldUnlock)
        rawFile->unsafeUnlock();
    return ret;
}

std::string BFile::getPath() const {
    return rawFile->path;
}

// Implement operations for the file
Data* BFile::implement(const OperationType operation, BuiltinArgs* args) {
    if (operation == AT && args->size == 2 && args->arg1->getType() == INT) {
        int lineNum = ((Integer*)args->arg1)->getValue();
        rawFile->lock();
        if (lineNum < 0 || lineNum >= rawFile->contents.size()) {
            int endrange = rawFile->contents.size();
            rawFile->unlock();
            bberror("Line number " + std::to_string(lineNum) + " out of range [0," + std::to_string(endrange)+ ")");
            return nullptr;
        }
        std::string lineContent = rawFile->contents[lineNum];
        rawFile->unlock();
        STRING_RESULT(lineContent);
    }
    if (operation == PUT && args->size == 3 && args->arg1->getType() == INT && args->arg2->getType() == STRING) {
        int lineNum = ((Integer*)args->arg1)->getValue();
        std::string newContent = args->arg2->toString();
        rawFile->lock();
        if (lineNum < 0 || lineNum >= rawFile->contents.size()) {
            int endlineNum = rawFile->contents.size();
            rawFile->unlock();
            bberror("Line number " + std::to_string(lineNum) + " out of range [0," + std::to_string(endlineNum) + ")");
            return nullptr;
        }
        rawFile->contents[lineNum] = newContent;
        rawFile->unlock();
        return nullptr;
    }
    if (operation == LEN) {
        rawFile->lock();
        int ret = rawFile->contents.size();
        rawFile->unlock();
        INT_RESULT(ret);
    }
    if (operation == TOCOPY || operation == TOFILE) {
        return shallowCopy();
    }
    if (operation == TOSTR) {
        STRING_RESULT(toString());
    }
    if (operation == TOITER) {
        return new Iterator(std::make_shared<IteratorContents>(shallowCopy()));
    }
    throw Unimplemented();
}
