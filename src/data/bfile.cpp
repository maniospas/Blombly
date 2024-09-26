#include "data/BFile.h"
#include "data/Integer.h"
#include "data/BString.h"
#include "data/Iterator.h"
#include "common.h"
#include <iostream>
#include <fstream>


BFile::BFile(const std::string& path_) : path(path_), size(0), lockable(0) {
    if (pthread_mutex_init(&memoryLock, nullptr) != 0) {
        bberror("Failed to create a mutex for file read/write");
    }
    std::ifstream file(path);
    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line)) {
            contents.push_back(line);
        }
        file.close();
    } else {
        bberror("Failed to open file: " + path);
    }
    size = contents.size();
}

int BFile::getType() const {
    return FILETYPE;
}

std::string BFile::toString() const {
    std::string result = "file@" + path + ":";
    for (std::size_t i = 0; i < std::min(static_cast<std::size_t>(5), contents.size()); ++i) {
        result += "\n" + contents[i];
    }
    if (contents.size() > 5) {
        result += "\n...";
    }
    return result;
}

std::string BFile::getPath() const {
    return path;
}

std::shared_ptr<Data> BFile::shallowCopy() const {
    auto ret = std::make_shared<BFile>(path);
    return ret;
}

std::shared_ptr<Data> BFile::implement(const OperationType operation, BuiltinArgs* args) {
    if (operation == AT && args->size == 2 && args->arg1->getType() == INT) {
        int lineNum = static_cast<Integer*>(args->arg1.get())->getValue();
        if (lineNum < 0 || lineNum >= contents.size()) {
            bberror("Line number " + std::to_string(lineNum) + " out of range [0," + std::to_string(contents.size()) + ")");
            return nullptr;
        }
        std::string lineContent = contents[lineNum];
        STRING_RESULT(lineContent);
    }
    if (operation == PUT && args->size == 3 && args->arg1->getType() == INT && args->arg2->getType() == STRING) {
        int lineNum = static_cast<Integer*>(args->arg1.get())->getValue();
        std::string newContent = args->arg2->toString();
        if (lineNum < 0 || lineNum >= contents.size()) {
            bberror("Line number " + std::to_string(lineNum) + " out of range [0," + std::to_string(contents.size()) + ")");
            return nullptr;
        }
        contents[lineNum] = newContent;
        return nullptr;
    }
    if (operation == LEN) {
        int ret = contents.size();
        INT_RESULT(ret);
    }
    if (operation == TOCOPY || operation == TOFILE) {
        return shallowCopy();
    }
    if (operation == TOSTR) {
        STRING_RESULT(toString());
    }
    if (operation == TOITER) {
        return std::make_shared<Iterator>(args->arg0);
    }
    throw Unimplemented();
}
