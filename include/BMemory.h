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
        callId = getId("\\call");
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
    tsl::hopscotch_map<int, Data*> data;
    tsl::hopscotch_set<int> finals;
    int fastId;
    Data* fastData;

public:
    void release();
    BMemory* parent;
    tsl::hopscotch_set<Future*> attached_threads;
    bool allowMutables;

    bool isOrDerivedFrom(BMemory* memory) const;
    void leak();

    explicit BMemory(BMemory* par, int expectedAssignments);
    ~BMemory();

    bool contains(int item);
    Data* get(int item); // allowMutable = true
    Data* get(int item, bool allowMutable);
    Data* getShallow(int item);
    Data* getOrNull(int item, bool allowMutable);
    Data* getOrNullShallow(int item);
    void unsafeSet(BMemory* handler, int item, Data* value, Data* prev);
    void unsafeSet(int item, Data* value, Data* prev);
    Data* unsafeSet(int item, Data* value);
    int size() const;
    void removeWithoutDelete(int item);
    void setFinal(int item);
    bool isFinal(int item) const;

    void pull(BMemory* other);
    void replaceMissing(BMemory* other);
    void detach();
    void detach(BMemory* par);

    static void verify_noleaks();
};

#endif // MEMORY_H
