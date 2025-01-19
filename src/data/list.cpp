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
    std::string result = "";
    std::lock_guard<std::recursive_mutex> lock(memoryLock);
    int64_t n = contents.size();
    for(int64_t i=front;i<n;++i) {
        if(result.size()) result += ", ";
        if(contents[i].exists()) result += contents[i]->toString(memory); 
        else result += contents[i].torepr();
    }
    return result;
}

DataPtr BList::at(int64_t index) const {
    if (index < 0) return OUT_OF_RANGE;
    std::lock_guard<std::recursive_mutex> lock(memoryLock);
    index += front;
    if (index>=contents.size()) return OUT_OF_RANGE;
    auto res = contents.at(index);
    return res;
}

Vector* BList::toVector(BMemory* memory) const {
    int64_t n = contents.size();
    Vector* vec = new Vector(n, false);
    double* rawret = vec->data;
    std::lock_guard<std::recursive_mutex> lock(memoryLock);
    try {
        for (int64_t i = front; i < n; ++i) {
            const DataPtr& content = contents[i];
            if (content.isint()) rawret[i-front] = (double)content.unsafe_toint();
            else if (content.isfloat()) rawret[i-front] = content.unsafe_tofloat();
            else bberror("Non-numeric value in list during conversion to vector (float or int expected)");
        }
    } 
    catch (BBError& e) {throw e;}
    return vec;
}

BHashMap* BList::toMap() const {
    BHashMap* map = new BHashMap();
    std::lock_guard<std::recursive_mutex> lock(memoryLock);
    int64_t n = contents.size();
    for (int64_t i = front; i < n; ++i) {
        const DataPtr& content = contents[i];
        bbassert(content.existsAndTypeEquals(LIST), "Can only create a map from a list of key,value pairs (list of two-element lists)");
        BList* list = static_cast<BList*>(contents[i].get());
        if (list->contents.size() != 2) bberror("Can only create a map from a list of key,value pairs (list of two-element lists)");
        map->put(list->contents[0], list->contents[1]);
    }
    return map;
}

Result BList::add(BMemory* memory, const DataPtr& other_) {
    if(!other_.existsAndTypeEquals(LIST)) return Data::add(memory, other_);
    BList* other = static_cast<BList*>(other_.get());
    std::lock_guard<std::recursive_mutex> lock(memoryLock);
    std::lock_guard<std::recursive_mutex> otherLock(other->memoryLock);
    BList* ret = new BList(contents.size()+other->contents.size());
    ret->contents.insert(ret->contents.end(), contents.begin(), contents.end());
    ret->contents.insert(ret->contents.end(), other->contents.begin(), other->contents.end());
    for(const DataPtr& dat : ret->contents) dat.existsAddOwner();
    return std::move(Result(ret));
}

Result BList::push(BMemory* memory, const DataPtr& other) {
    bbassert(other.islitorexists(), "Cannot push a missing value to a list");
    if(other.existsAndTypeEquals(ERRORTYPE)) bberror("Cannot push an error to a list");
    other.existsAddOwner();
    std::lock_guard<std::recursive_mutex> lock(memoryLock);
    contents.emplace_back(other);
    return std::move(Result(this));
}

Result BList::pop(BMemory* memory) {
    std::lock_guard<std::recursive_mutex> lock(memoryLock);
    if (contents.empty()) return std::move(Result(OUT_OF_RANGE));
    const auto& element = contents.back();
    auto ret = Result(element);
    contents.pop_back();
    element.existsRemoveFromOwner();
    return std::move(ret);
}

Result BList::next(BMemory* memory) {
    std::lock_guard<std::recursive_mutex> lock(memoryLock);
    if (front >= contents.size()) return std::move(Result(OUT_OF_RANGE));
    const auto& element = contents[front];
    auto ret = Result(element);
    front++;
    resizeContents();
    element.existsRemoveFromOwner();
    return std::move(ret);
}

Result BList::at(BMemory* memory, const DataPtr& other) {
    if (other.isint()) {
        int64_t index = other.unsafe_toint();
        if (index < 0) return std::move(Result(OUT_OF_RANGE));
        index += front;
        if (index>=contents.size()) return std::move(Result(OUT_OF_RANGE));
        std::lock_guard<std::recursive_mutex> lock(memoryLock);
        const DataPtr& res = contents[index];
        return std::move(Result(res));
    } 
    if (other.existsAndTypeEquals(STRUCT) || other.existsAndTypeEquals(LIST) || other.existsAndTypeEquals(ITERATOR)) {
        std::lock_guard<std::recursive_mutex> lock(memoryLock);
        Result iter = other->iter(memory);
        const auto& iterator = iter.get();
        bbassert(iterator.existsAndTypeEquals(ITERATOR), "Can only find list indexes based on an iterable object, but a non-iterable struct was provided.");
        Iterator* iterPtr = static_cast<Iterator*>(iterator.get());
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
        BList* ret = new BList(contents.size());
        ret->contents.reserve(iterPtr->expectedSize());
        while (true) {
            Result next = iterator->next(memory);
            const auto& indexData = next.get();
            if (indexData==OUT_OF_RANGE) break;
            bbassert(indexData.isint(), "Iterable list indexes can only contain integers.");
            int64_t id = indexData.unsafe_toint();
            if (id < 0) return std::move(Result(OUT_OF_RANGE));
            id += front;
            if (id>=contents.size()) return std::move(Result(OUT_OF_RANGE));
            const auto& element = contents[id];
            element.existsAddOwner();
            ret->contents.emplace_back(element);
        }
        return std::move(Result(ret));
    }
    return Data::at(memory, other);
}

Result BList::put(BMemory* memory, const DataPtr& position, const DataPtr& value) {
    if(!position.isint()) return Data::put(memory, position, value);
    int64_t index = position.unsafe_toint();
    if (index < 0) return std::move(Result(OUT_OF_RANGE));
    std::lock_guard<std::recursive_mutex> lock(memoryLock);
    index += front;
    if (index>=contents.size()) return std::move(Result(OUT_OF_RANGE));
    bbassert(value.islitorexists(), "Cannot set a missing value on a list");
    if(value.existsAndTypeEquals(ERRORTYPE)) bberror("Cannot set an error on a list");
    DataPtr prev = contents[index];
    contents[index] = value;
    value.existsAddOwner();
    prev.existsRemoveFromOwner();
    return std::move(Result(nullptr));
}

int64_t BList::len(BMemory* memory) {
    std::lock_guard<std::recursive_mutex> lock(memoryLock);
    return contents.size()-front;
}

Result BList::iter(BMemory* memory) {
    std::lock_guard<std::recursive_mutex> lock(memoryLock);
    return std::move(Result(new AccessIterator(this, contents.size()-front)));
}

void BList::clear(BMemory* memory)  {
    std::lock_guard<std::recursive_mutex> lock(memoryLock);
    int64_t n = contents.size();
    for(int64_t i=front;i<n;++i) contents[i].existsRemoveFromOwner();
    contents = std::vector<DataPtr>();
    front = 0;
}

Result BList::move(BMemory* memory) {
    BList* ret = new BList();
    std::lock_guard<std::recursive_mutex> lock(memoryLock);
    ret->front = front;
    ret->contents = std::move(contents);
    contents.clear();
    front = 0;
    return std::move(Result(ret));
}

void BList::resizeContents() {
    if(front>32 && front>contents.size()/3) {
        contents.erase(contents.begin(), contents.begin() + front);
        front = 0;
    }
}