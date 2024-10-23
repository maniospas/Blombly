#include "data/BFile.h"
#include "data/Integer.h"
#include "data/BString.h"
#include "data/Iterator.h"
#include "data/List.h"
#include "common.h"
#include <iostream>
#include <fstream>


BFile::BFile(const std::string& path_) : path(path_), size(0), Data(FILETYPE) {
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

Result BFile::implement(const OperationType operation, BuiltinArgs* args) {
    if (operation == AT && args->size == 2 && args->arg1->getType() == BB_INT) {
        int lineNum = static_cast<Integer*>(args->arg1)->getValue();
        if (lineNum < 0 || lineNum >= contents.size()) {
            bberror("Line number " + std::to_string(lineNum) + " out of range [0," + std::to_string(contents.size()) + ")");
            return std::move(Result(nullptr));
        }
        std::string lineContent = contents[lineNum];
        STRING_RESULT(lineContent);
    }
    /*if (operation == PUT && args->size == 3 && args->arg1->getType() == BB_INT && args->arg2->getType() == STRING) {
        int lineNum = static_cast<Integer*>(args->arg1)->getValue();
        std::string newContent = args->arg2->toString();
        if (lineNum < 0 || lineNum >= contents.size()) {
            bberror("Line number " + std::to_string(lineNum) + " out of range [0," + std::to_string(contents.size()) + ")");
            return std::move(Result(nullptr));
        }
        contents[lineNum] = newContent;
        return std::move(Result(nullptr));
    }*/
    if (args->size == 1) {
        if (operation == LEN) {
            int ret = contents.size();
            BB_INT_RESULT(ret);
        }
        if (operation == TOFILE) {
            return std::move(Result(this));
        }
        if (operation == TOSTR) {
            STRING_RESULT(toString());
        }
        if (operation == TOITER) {
            return std::move(Result(new AccessIterator(args->arg0)));
        }
        if (operation == TOLIST) {
            int n = contents.size();
            BList* list = new BList(n);
            for(int i=0;i<n;++i) {
                BString* element = new BString(contents[i]);
                element->addOwner();
                element->leak();
                list->contents.push_back(element);
            }
            return std::move(Result(list));
        }
    }
    throw Unimplemented();
}
