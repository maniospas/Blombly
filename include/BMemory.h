#ifndef BMEMORY_H
#define BMEMORY_H

#include <memory>
#include <unordered_map>
#include <string>
#include <stack>
#include <vector>
#include <iostream>
#include <atomic>
#include "data/Data.h"
#include "tsl/hopscotch_map.h"
#include "tsl/hopscotch_set.h"
class Future;
class Struct;
class Command;


class VariableManager {
private:
    tsl::hopscotch_map<std::string, int> registeredSymbols;
    tsl::hopscotch_map<int, std::string> registeredIds;
public:
    static constexpr int thisId = 0;
    static constexpr int argsId = 1;
    static constexpr int noneId = 2;
    static constexpr int callId = 3;

    VariableManager() {
        getId("this");
        getId("args");
        getId("#");
        getId("call");
    }
    int getId(const std::string& symbol) {
        if(registeredSymbols.find(symbol) == registeredSymbols.end()) {
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

class BMemory {
private:
    tsl::hopscotch_map<int, int> data;
    std::vector<DataPtr> contents;
    void unsafeSet(int item, const DataPtr& value);
    std::vector<Code*> finally;
    inline int find(int item) const;
    static const int end = -1;
    int first_item;
    int max_cache_size;
    bool hasAtLeastOneFinal;
public:
    inline BMemory* getParentWithFinals() {
        // this is used to skip intermediate useless memory contexts in get() within function 
        if(hasAtLeastOneFinal) return this; 
        if(parent) return parent->getParentWithFinals();
        return nullptr;
    }
    tsl::hopscotch_map<Code*, Struct*> codeOwners;
    BMemory* parent;
    tsl::hopscotch_set<Future*> attached_threads;

    void release();
    bool allowMutables;

    explicit BMemory(BMemory* par, int expectedAssignments, DataPtr thisObject=nullptr);
    ~BMemory();

    const DataPtr& get(int item); // allowMutable = true
    const DataPtr& get(int item, bool allowMutable);
    const DataPtr& getShallow(int item);
    const DataPtr& getOrNull(int item, bool allowMutable);
    const DataPtr& getOrNullShallow(int item);
    void directTransfer(int to, int from);
    void set(int item, const DataPtr& value);
    void setFuture(int item, const DataPtr& value);
    void unsafeSetLiteral(int item, const DataPtr& value);
    void setToNullIgnoringFinals(int item);
    int size() const;
    void setFinal(int item);

    void pull(BMemory* other);
    void replaceMissing(BMemory* other);
    void await();
    void detach(BMemory* par);
    void runFinally();
    void addFinally(Code* code);

    static void verify_noleaks();
    friend class Struct;
};

#endif // MEMORY_H
