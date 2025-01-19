#include "data/List.h"
#include "data/Integer.h"
#include "data/Iterator.h"
#include "data/BFloat.h"
#include "data/Vector.h"
#include "data/BString.h"
#include "data/BHashMap.h"
#include "data/BError.h"
#include "common.h"
#include <iostream>
#include <mutex>

extern BError* OUT_OF_RANGE;

// BList constructor
BList::BList() : Data(LIST), front(0) {}
BList::BList(int64_t reserve) : Data(LIST), front(0)  {contents.reserve(reserve);}
BList::~BList() {
    int64_t n = contents.size();
    for(int64_t i=front;i<n;++i) contents[i].existsRemoveFromOwner();
}

std::string BList::toString(BMemory* memory){
    std::lock_guard<std::recursive_mutex> lock(memoryLock);
    std::string result = "[";
    int64_t n = contents.size();
    for(int64_t i=front;i<n;++i) {
        if (result.size()!=1) result += ", ";
        if(contents[i].exists()) result += contents[i]->toString(memory); else result += contents[i].torepr();
    }
    return result + "]";
}

DataPtr BList::at(int64_t index) const {
    std::lock_guard<std::recursive_mutex> lock(memoryLock);
    if (index < 0) return OUT_OF_RANGE;
    index += front;
    if (index>=contents.size()) return OUT_OF_RANGE;
    auto res = contents.at(index);
    return res;
}

Result BList::implement(const OperationType operation, BuiltinArgs* args, BMemory* memory) {
    std::lock_guard<std::recursive_mutex> lock(memoryLock);
    
    if (args->size == 1) {
        switch (operation) {
            case LEN: {
                int res = contents.size()-front;
                return std::move(Result(res));
            }
            case TOITER: return std::move(Result(new AccessIterator(args->arg0, contents.size()-front)));
            case NEXT: {
                if (front >= contents.size()) return std::move(Result(OUT_OF_RANGE));
                const auto& element = contents[front];
                auto ret = Result(element);
                front++;
                resizeContents();
                element.existsRemoveFromOwner();
                return std::move(ret);
            }
            case POP: {
                if (contents.empty()) return std::move(Result(OUT_OF_RANGE));
                const auto& element = contents.back();
                auto ret = Result(element);
                contents.pop_back();
                element.existsRemoveFromOwner();
                return std::move(ret);
            }
            case CLEAR : {
                int64_t n = contents.size();
                for(int64_t i=front;i<n;++i) contents[i].existsRemoveFromOwner();
                contents = std::vector<DataPtr>();
                front = 0;
                return std::move(Result(nullptr));
            }
            case TOMAP: {
                auto map = new BHashMap();
                int64_t n = contents.size();
                for (int64_t i = front; i < n; ++i) {
                    bbassert(contents[i]->getType()==LIST, "Can only create a map from a list of key,value pairs");
                    BList* list = static_cast<BList*>(contents[i].get());
                    if (!list || list->contents.size() != 2) bberror("Can only create a map from a list of key,value pairs");
                    map->put(list->contents[0], list->contents[1]);
                }
                return std::move(Result(map));
            }
            case TOVECTOR: {
                int64_t n = contents.size();
                auto vec = new Vector(n, false);
                double* rawret = vec->data;
                BuiltinArgs args;
                //args.preallocResult = new BFloat(0);
                args.size = 1;
                try {
                    for (int64_t i = front; i < n; ++i) {
                        args.arg0 = contents[i];
                        Result tempValue = args.arg0->implement(TOBB_FLOAT, &args, memory);
                        DataPtr temp = tempValue.get();
                        auto type = temp->getType();
                        if (type == BB_INT) rawret[i-front] = static_cast<Integer*>(temp.get())->getValue();
                        else if (type == BB_FLOAT) rawret[i-front] = static_cast<BFloat*>(temp.get())->getValue();
                        else bberror("Non-numeric value in list during conversion to vector (float or int expected)");
                    }
                } 
                catch (BBError& e) {
                    throw e;
                }
                return std::move(Result(vec));
            }
            case MOVE: {
                BList* ret = new BList();
                ret->front = front;
                ret->contents = std::move(contents);
                contents.clear();
                front = 0;
                return std::move(Result(ret));
            }
        }
        throw Unimplemented();
    }

    if (operation == AT && args->size == 2) {
        if (args->arg1.isint()) {
            int64_t index = args->arg1.unsafe_toint();
            if (index < 0) return std::move(Result(OUT_OF_RANGE));
            index += front;
            if (index>=contents.size()) return std::move(Result(OUT_OF_RANGE));
            DataPtr res = contents[index];
            return std::move(Result(res));
        } 
        else if (args->arg1.existsAndTypeEquals(STRUCT) || args->arg1.existsAndTypeEquals(LIST) || args->arg1.existsAndTypeEquals(ITERATOR)) {
            BuiltinArgs implargs;
            implargs.size = 1;
            implargs.arg0 = args->arg1;

            Result iter = args->arg1->implement(TOITER, &implargs, memory);
            const auto& iterator = iter.get();
            bbassert(iterator.existsAndTypeEquals(ITERATOR), "Can only find list indexes based on an iterable object, but a non-iterable struct was provided.");

            Iterator* iterPtr = static_cast<Iterator*>(iterator.get());

            // Handle contiguous iterators efficiently
            if (iterPtr->isContiguous()) {
                int64_t start = iterPtr->getStart();
                int64_t end = iterPtr->getEnd();
                if (start < 0) return std::move(Result(OUT_OF_RANGE));
                start += front;
                if (start>=contents.size()) return std::move(Result(OUT_OF_RANGE));
                if (end < 0) return std::move(Result(OUT_OF_RANGE));
                end += front;
                if (end>=contents.size()) return std::move(Result(OUT_OF_RANGE));
                BList* ret = new BList(end - start);
                for (int64_t i = start; i < end; ++i) {
                    DataPtr element = contents[i];
                    element.existsAddOwner();
                    ret->contents.emplace_back(element);
                }
                return std::move(Result(ret));
            } 
            else {
                BList* ret = new BList(contents.size());
                ret->contents.reserve(iterPtr->expectedSize());

                while (true) {
                    implargs.size = 1;
                    Result next = iterator->implement(NEXT, &implargs, memory);
                    DataPtr indexData = next.get();
                    if (indexData==OUT_OF_RANGE) break;
                    bbassert(indexData.isint(), "Iterable list indexes can only contain integers.");
                    int64_t id = indexData.unsafe_toint();
                    if (id < 0) return std::move(Result(OUT_OF_RANGE));
                    id += front;
                    if (id>=contents.size()) return std::move(Result(OUT_OF_RANGE));
                    DataPtr element = contents[id];
                    element.existsAddOwner();
                    ret->contents.emplace_back(element);
                }
                return std::move(Result(ret));
            }
        }
    }

    if (operation == PUSH && args->size == 2 && args->arg0 == this) {
        auto value = args->arg1;
        bbassert(value.islitorexists(), "Cannot push a missing value to a list");
        if(value.existsAndTypeEquals(ERRORTYPE)) bberror("Cannot push an error to a list");
        value.existsAddOwner();
        contents.emplace_back(value);
        return std::move(Result(this));
    }

    if (operation == PUT && args->size == 3 && args->arg1.isint()) {
        int64_t index = args->arg1.unsafe_toint();
        if (index < 0) bberror("Out of range");//return std::move(Result(OUT_OF_RANGE));
        index += front;
        if (index>=contents.size()) bberror("Out of range");//return std::move(Result(OUT_OF_RANGE));
        auto value = args->arg2;
        bbassert(value.islitorexists(), "Cannot set a missing value on a list");
        if(value.existsAndTypeEquals(ERRORTYPE)) bberror("Cannot set an error on a list");
        DataPtr prev = contents[index];
        contents[index] = value;
        value.existsAddOwner();
        prev.existsRemoveFromOwner();
        return std::move(Result(nullptr));
    }
    
    if (operation == ADD && args->size == 2 && args->arg1.existsAndTypeEquals(LIST) && args->arg0==this) {
        BList* other = static_cast<BList*>(args->arg1.get());
        std::lock_guard<std::recursive_mutex> otherLock(other->memoryLock);
        BList* ret = new BList(contents.size()+other->contents.size());
        ret->contents.insert(ret->contents.end(), contents.begin(), contents.end());
        ret->contents.insert(ret->contents.end(), other->contents.begin(), other->contents.end());
        for(const DataPtr& dat : ret->contents) dat.existsAddOwner();
        return std::move(Result(ret));
    }

    throw Unimplemented();
}

void BList::resizeContents() {
    if(front>32 && front>contents.size()/3) {
        contents.erase(contents.begin(), contents.begin() + front);
        front = 0;
    }
}