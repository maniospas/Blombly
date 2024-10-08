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
    tsl::hopscotch_map<int, std::shared_ptr<Data>> data;
    tsl::hopscotch_set<int> finals;
    std::weak_ptr<Data> fastLastAccess;
    int fastLastAccessId;

public:
    void release();
    std::shared_ptr<BMemory> parent;
    tsl::hopscotch_set<std::shared_ptr<Future>> attached_threads;
    bool allowMutables;

    bool isOrDerivedFrom(const BMemory* memory) const;

    explicit BMemory(const std::shared_ptr<BMemory>& par, int expectedAssignments);
    ~BMemory();

    bool contains(int item);
    std::shared_ptr<Data> get(int item); // allowMutable = true
    std::shared_ptr<Data> get(int item, bool allowMutable);
    std::shared_ptr<Data> getShallow(int item);
    std::shared_ptr<Data> getOrNull(int item, bool allowMutable);
    std::shared_ptr<Data> getOrNullShallow(int item);
    void unsafeSet(const std::shared_ptr<BMemory>& handler, int item, const std::shared_ptr<Data>& value, const std::shared_ptr<Data>& prev);
    void unsafeSet(int item, const std::shared_ptr<Data>& value, const std::shared_ptr<Data>& prev);
    std::shared_ptr<Data> unsafeSet(int item, const std::shared_ptr<Data>& value);
    int size() const;
    void removeWithoutDelete(int item);
    void setFinal(int item);
    bool isFinal(int item) const;

    void pull(const std::shared_ptr<BMemory>& other);
    void replaceMissing(const std::shared_ptr<BMemory>& other);
    void detach();
    void detach(const std::shared_ptr<BMemory>& par);

    static void verify_noleaks();
};

#endif // MEMORY_H
