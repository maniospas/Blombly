#include "BMemory.h"
#include "common.h"
#include "memory.h"
#include "data/BError.h"
#include "data/Future.h"
#include "data/Code.h"
#include "data/Struct.h"
#include "interpreter/functional.h"
#include <unordered_set>  

std::atomic<unsigned long long> countUnrealeasedMemories(0);
extern VariableManager variableManager;

void BMemory::verify_noleaks() {
    //bbassert(memories.size() == 0, "There are " + std::to_string(memories.size()) + " leftover memory contexts leaked");
    int countUnreleased = countUnrealeasedMemories.load();
    countUnrealeasedMemories = 0;
    bbassert(countUnreleased == 0, "There are " + std::to_string(countUnreleased) + " leftover memory contexts leaked");  // the main memory is a global object (needed to sync threads on errors)
}

BMemory::BMemory(BMemory* par, int expectedAssignments, DataPtr thisObject) : parent(par), allowMutables(true), first_item(INT_MAX) {  // we are determined to never use special symbols as the start of our cache index
    //std::cout << "created "<<this<<"\n";
    ++countUnrealeasedMemories;
    //data.reserve(expectedAssignments);
    contents.resize(expectedAssignments, DataPtr::NULLP);
    max_cache_size = expectedAssignments;
}

void BMemory::release() {
    //std::cout << "releasing "<<this<<"\n";
    std::string destroyerr = "";
    for (const auto& thread : attached_threads) {
        try {Result res = thread->getResult();}
        catch (const BBError& e) {destroyerr += std::string(e.what())+"\n";}
        thread->removeFromOwner();
    }
    
    attached_threads.clear();
    try {runFinally();}
    catch (const BBError& e) {destroyerr += std::string(e.what())+"\n";}

    for (const auto& thread : attached_threads) {
        try {thread->getResult();}
        catch (const BBError& e) {destroyerr += std::string(e.what())+"\n";}
        thread->removeFromOwner();
    }
    attached_threads.clear();

    for (const auto& dat : contents) {
        try {
            if(dat.existsAndTypeEquals(ERRORTYPE) && !static_cast<BError*>(dat.get())->isConsumed()) destroyerr += "\033[0m(\x1B[31m ERROR \033[0m) The following error was caught but never handled:\n"+dat->toString(this)+"\n";
            if(dat.exists()) dat->removeFromOwner();
        }
        catch (const BBError& e) {destroyerr += std::string(e.what())+"\n";}
    }
    data.clear();
    contents.clear();
    if(destroyerr.size())  throw BBError(destroyerr.substr(0, destroyerr.size()-1));
}



BMemory::~BMemory() {
    contents[data[variableManager.thisId]] = DataPtr::NULLP;
    countUnrealeasedMemories--; 
    release();
}

int BMemory::size() const {
    return data.size();
}

int BMemory::find(int item) const {
    // last_fast_item checks runs first to priorize the more likely comparison
    if(item>=first_item && item-first_item<max_cache_size) return item-first_item;
    const auto& idx = data.find(item);
    if(idx==data.end()) return end;
    return idx->second;
}

const DataPtr& BMemory::get(int item) { // allowMutable = true
    int idx = find(item);
    if(idx==end) bberror("Missing value"+std::string(size()?"":" in cleared memory ")+": " + variableManager.getSymbol(item));
    const auto& ret = contents[idx];
    if (ret.existsAndTypeEquals(FUTURE)) {
        auto prevRet = static_cast<Future*>(ret.get());
        auto resVal = prevRet->getResult();
        unsafeSet(item, resVal.get()); 
        attached_threads.erase(prevRet);
        prevRet->removeFromOwner();
        return get(item);
    }
    if (!ret.islitorexists()) {
        bbassert(parent, "Missing value"+std::string(size()?"":" in cleared memory ")+": " + variableManager.getSymbol(item));
        if(parent) return parent->get(item, allowMutables);
    }
    return ret;
}

const DataPtr& BMemory::get(int item, bool allowMutable) {
    int idx = find(item);
    if(idx==end) bberror("Missing value"+std::string(size()?"":" in cleared memory ")+": " + variableManager.getSymbol(item));
    const auto& ret = contents[idx];
    if (ret.existsAndTypeEquals(FUTURE)) {
        auto prevRet = static_cast<Future*>(ret.get());
        auto resVal = prevRet->getResult();
        unsafeSet(item, resVal.get()); 
        attached_threads.erase(prevRet);
        prevRet->removeFromOwner();
        bbassert(ret.islitorexists(), "Missing value"+std::string(size()?"":" in cleared memory ")+": " + variableManager.getSymbol(item));
        return get(item, allowMutable);
    }
    if (ret.islitorexists()) {bbassert(allowMutable || isFinal(item), "Mutable symbol cannot be requested from another scope: " + variableManager.getSymbol(item));}
    else if (parent) return parent->get(item, allowMutables && allowMutable);
    bbassert(ret.islitorexists(), "Missing value"+std::string(size()?"":" in cleared memory ")+": " + variableManager.getSymbol(item));
    return ret;
}

const DataPtr& BMemory::getShallow(int item) {
    int idx = find(item);
    if(idx==end) bberror("Missing value"+std::string(size()?"":" in cleared memory ")+": " + variableManager.getSymbol(item));
    const auto& ret = contents[idx];
    if (ret.existsAndTypeEquals(FUTURE)) {
        //std::cout << "here4\n";
        auto prevRet = static_cast<Future*>(ret.get());
        auto resVal = prevRet->getResult();
        unsafeSet(item, resVal.get());
        attached_threads.erase(prevRet);prevRet->removeFromOwner();
        return getShallow(item);
    }
    if(!ret.islitorexists()) bberror("Missing value"+std::string(size()?"":" in cleared memory ")+": " + variableManager.getSymbol(item));
    return ret;
}

const DataPtr& BMemory::getOrNullShallow(int item) {
    int idx = find(item);
    if(idx==end) return DataPtr::NULLP;
    const auto& ret = contents[idx];
    if (ret.existsAndTypeEquals(FUTURE)) {
        auto prevRet = static_cast<Future*>(ret.get());
        auto resVal = prevRet->getResult();
        unsafeSet(item, resVal.get());
        attached_threads.erase(prevRet);prevRet->removeFromOwner();
        return contents[idx];
    }
    return ret;
}

const DataPtr& BMemory::getOrNull(int item, bool allowMutable) {
    int idx = find(item);
    if(idx==end) return DataPtr::NULLP;
    const auto& ret = contents[idx];
    if (ret.existsAndTypeEquals(FUTURE)) {
        auto prevRet = static_cast<Future*>(ret.get());
        auto resVal = prevRet->getResult();
        unsafeSet(item, resVal.get());
        attached_threads.erase(prevRet);prevRet->removeFromOwner();
        return contents[idx];
    }
    if (ret.islitorexists()) {bbassert(allowMutable || isFinal(item), "Mutable symbol cannot be accessed from a nested block: " + variableManager.getSymbol(item));}
    else if (parent) return parent->getOrNull(item, allowMutables && allowMutable);
    return ret;
}


void BMemory::unsafeSetLiteral(int item, const DataPtr& value) { // when this is called, we are guaranteed that value.exists() == false
    int idx = find(item);
    if(idx==end) {
        if(first_item==INT_MAX && item>=first_item) {
            first_item = item;
            contents[0] = value;
            return;
        }
        data[item] = contents.size();
        contents.push_back(value);
        return;
    }
    auto& prev = contents[idx];
    if(prev.exists()) {
        if(isFinal(item)) bberror("Cannot overwrite final value: " + variableManager.getSymbol(item));
        if(prev->getType()==ERRORTYPE && !static_cast<BError*>(prev.get())->isConsumed()) bberror("Trying to overwrite an unhandled error:\n"+prev->toString(this));
        prev->removeFromOwner();
    }
    prev = value;
}

void BMemory::set(int item, const DataPtr& value) {
    int idx = find(item);
    if(idx==end) {
        if(value.exists()) value->addOwner();
        if(first_item==INT_MAX && item>=first_item) {
            first_item = item;
            contents[0] = value;
            return;
        }
        data[item] = contents.size();
        contents.push_back(value);
        return;
    }
    auto& prev = contents[idx];
    if(prev.exists()){
        if(isFinal(item)) bberror("Cannot overwrite final value: " + variableManager.getSymbol(item));
        if(prev->getType()==ERRORTYPE && !static_cast<BError*>(prev.get())->isConsumed()) bberror("Trying to overwrite an unhandled error:\n"+prev->toString(this));
    }
    if(value.exists()) value->addOwner();
    if(prev.exists()) prev->removeFromOwner();
    prev = value;
}

void BMemory::unsafeSet(int item, const DataPtr& value) {
    int idx = find(item);
    if(idx==end) {
        if(value.exists()) value->addOwner();
        if(first_item==INT_MAX && item>=first_item) {
            first_item = item;
            contents[0] = value;
            return;
        }
        data[item] = contents.size();
        contents.push_back(value);
        return;
    }
    DataPtr& prev = contents[idx];
    if(prev.existsAndTypeEquals(ERRORTYPE) && !static_cast<BError*>(prev.get())->isConsumed()) bberror("Trying to overwrite an unhandled error:\n"+prev->toString(this));
    if(value.exists()) value->addOwner();
    if(prev.exists()) prev->removeFromOwner();
    prev = value;
}

void BMemory::setFinal(int item) {finals.insert(item);}
bool BMemory::isFinal(int item) const {return finals.find(item) != finals.end();}

void BMemory::pull(BMemory* other) {
    for (const auto& it : other->data) {
        const auto& dat = other->contents[it.second];
        if (dat.islitorexists() && it.first!=variableManager.thisId) set(it.first, it.second);
    }
}

void BMemory::replaceMissing(BMemory* other) {
    for (const auto& it : other->data) {
        int item = it.first;
        const auto& existing = getOrNullShallow(item);
        if (!existing.islitorexists()) {
            const auto& dat = contents[it.second];
            if (dat.islitorexists()) set(item, dat);
        }
    }
}

void BMemory::await() {
    if(attached_threads.size()==0) return; // we don't need to lock because on zero threads we are ok, on >=1 threads we don't care about the number
    std::string destroyerr = "";
    for (const auto& thread : attached_threads) {
        try {Result res = thread->getResult();}
        catch (const BBError& e) {destroyerr += std::string(e.what())+"\n";}
        thread->removeFromOwner();
    }
    attached_threads.clear();

    try {runFinally();}
    catch (const BBError& e) {destroyerr += std::string(e.what())+"\n";}

    for (const auto& thread : attached_threads) {
        try {thread->getResult();}
        catch (const BBError& e) {destroyerr += std::string(e.what())+"\n";}
        thread->removeFromOwner();
    }
    attached_threads.clear();

    for (const auto& dat : contents) {
        if (dat.existsAndTypeEquals(ERRORTYPE) && !static_cast<BError*>(dat.get())->isConsumed())  {
            static_cast<BError*>(dat.get())->consume();
            destroyerr += "\033[0m(\x1B[31m ERROR \033[0m) The following error was intercepted with `try` but never handled:\n"+dat->toString(this)+"\n";
        }
    }
    if(destroyerr.size()) throw BBError(destroyerr.substr(0, destroyerr.size()-1));
}

void BMemory::detach(BMemory* par) {
    await();
    parent = par;
}

void BMemory::runFinally() {
    std::string destroyerr = "";
    std::vector<Code*> tempFinally = std::move(finally);
    finally.clear();

    for(Code* code : tempFinally) {
        try {
            ExecutionInstance executor(code, this, true); // everything deferred runs after all threads have been synchronized, so stay in thread (last true argument)
            Result returnedValue = executor.run(code);
            bbassert(!executor.hasReturned(), "Cannot return a value from within `defer`");
        }
        catch (const BBError& e) {
            destroyerr += "\033[0m(\x1B[31m ERROR \033[0m) The following error occurred within `defer`:\n";
            destroyerr += std::string(e.what()) + "\n";
        }
    }

    bbassert(finally.size()==0, "Leftover `defer` handling.");
    if(destroyerr.size()) throw BBError(destroyerr.substr(0, destroyerr.size() - 1));
}


void BMemory::addFinally(Code* code) {
    finally.push_back(code);
}