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
    int thisId;
    int argsId;
    int noneId;
    int callId;
    VariableManager() {
        thisId = getId("this");
        argsId = getId("args");
        noneId = getId("#");
        callId = getId("call");
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
    void unsafeSet(int item, DataPtr value);
    std::vector<Code*> finally;
public:
    tsl::hopscotch_map<Code*, Struct*> codeOwners;
    tsl::hopscotch_set<int> finals;
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
    void set(int item, DataPtr value);
    void unsafeSetLiteral(int item, const DataPtr& value);
    int size() const;
    void setFinal(int item);
    bool isFinal(int item) const;

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
