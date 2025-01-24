#include "BMemory.h"
#include "common.h"
#include "memory.h"
#include "data/BError.h"
#include "data/Future.h"
#include "data/Code.h"
#include "data/Struct.h"
#include "interpreter/functional.h"
#include <unordered_set>  
#include <bit>
#include <xmmintrin.h>
#include <new>     // For std::align

std::atomic<unsigned long long> countUnrealeasedMemories(0);
extern VariableManager variableManager;

void BMemory::verify_noleaks() {
    //bbassert(memories.size() == 0, "There are " + std::to_string(memories.size()) + " leftover memory contexts leaked");
    int countUnreleased = countUnrealeasedMemories.load();
    countUnrealeasedMemories = 1;  // shared is always leftover
    bbassert(countUnreleased == 1, "There are " + std::to_string(countUnreleased-1) + " leftover memory contexts leaked");  // the main memory is a global object (needed to sync threads on errors)
}

BMemory::BMemory(BMemory* par, int expectedAssignments, DataPtr thisObject) : parent(par), allowMutables(true), first_item(INT_MAX), hasAtLeastOneFinal(false) {  // we are determined to never use special symbols as the start of our cache index
    ++countUnrealeasedMemories;
    //data.reserve(expectedAssignments);
    //contents.resize(expectedAssignments, DataPtr::NULLP);
    rawContents = (DataPtr*) aligned_alloc(expectedAssignments, sizeof(DataPtr));
    for(int i=0;i<expectedAssignments;++i) rawContents[i] = DataPtr::NULLP;
    rawContentsReserved = expectedAssignments;
    rawContentsSize = expectedAssignments;
    max_cache_size = expectedAssignments; 
}

void BMemory::release() {
    std::string destroyerr = "";
    for(const auto& thread : attached_threads) {
        try {Result res = thread->getResult();}
        catch (const BBError& e) {destroyerr += std::string(e.what())+"\n";}
        thread->removeFromOwner();
    }
    
    attached_threads.clear();
    try {runFinally();}
    catch(const BBError& e) {destroyerr += std::string(e.what())+"\n";}

    for(const auto& thread : attached_threads) {
        try {thread->getResult();}
        catch (const BBError& e) {destroyerr += std::string(e.what())+"\n";}
        thread->removeFromOwner();
    }
    attached_threads.clear();

    //for(const auto& dat : contents) {
    for(int i=0;i<rawContentsSize;++i) {
        const auto& dat = rawContents[i];
        try {
            if(dat.exists()) {
                Data* data = dat.get();
                if(data->getType()==(ERRORTYPE) && !static_cast<BError*>(data)->isConsumed()) destroyerr += "\033[0m(\x1B[31m ERROR \033[0m) The following error was caught but never handled:\n"+data->toString(this)+"\n";
                dat->removeFromOwner();
            }
        }
        catch(const BBError& e) {destroyerr += std::string(e.what())+"\n";}
    }
    data.clear();
    //contents.clear();
    if(rawContents) std::free(rawContents);
    rawContents = nullptr;
    rawContentsReserved = 0;
    rawContentsSize = 0;
    max_cache_size = 0;
    if(destroyerr.size())  throw BBError(destroyerr.substr(0, destroyerr.size()-1));
}

BMemory::~BMemory() {
    if(data.find(variableManager.thisId)!=data.end()) rawContents[data[variableManager.thisId]] = DataPtr::NULLP;
    countUnrealeasedMemories--; 
    release();
}

int BMemory::size() const {return data.size();}

const DataPtr& BMemory::resolveFuture(int item, const DataPtr& ret) {
    auto prevRet = static_cast<Future*>(ret.get());
    auto resVal = prevRet->getResult();
    unsafeSet(item, resVal.get()); 
    attached_threads.erase(prevRet);
    prevRet->removeFromOwner();
    return get(item); // Recurse to retrieve the updated value
}


const DataPtr& BMemory::get(int item, bool allowMutable) {
    int idx = find(item);
    if(idx==end) {
        if(parent) return parent->get(item, allowMutables);
        bberror("Missing value: " + variableManager.getSymbol(item));
    }
    const auto& ret = rawContents[idx];
    if(ret.existsAndTypeEquals(FUTURE)) [[unlikely]] {
        Future* prevRet = static_cast<Future*>(ret.get());
        Result resVal = prevRet->getResult();
        unsafeSet(item, resVal.get());
        attached_threads.erase(prevRet);
        prevRet->removeFromOwner();
        bbassert(ret.islitorexists(), "Missing value: " + variableManager.getSymbol(item));
        return get(item, allowMutable);
    }
    if(ret.islitorexists()) [[likely]] {bbassert(allowMutable || ret.isA(), "Non-final symbol found but cannot be accessed from another scope: " + variableManager.getSymbol(item));}
    else if(parent) return parent->get(item, allowMutables && allowMutable);
    else bberror("Missing value: " + variableManager.getSymbol(item));
    return ret;
}

const DataPtr& BMemory::getShallow(int item) {
    int idx = find(item);
    if(idx==end) bberror("Missing value: " + variableManager.getSymbol(item));
    const auto& ret = rawContents[idx];
    if(ret.existsAndTypeEquals(FUTURE)) [[unlikely]] {
        auto prevRet = static_cast<Future*>(ret.get());
        auto resVal = prevRet->getResult();
        unsafeSet(item, resVal.get());
        attached_threads.erase(prevRet);prevRet->removeFromOwner();
        return getShallow(item);
    }
    if(!ret.islitorexists()) bberror("Missing value: " + variableManager.getSymbol(item));
    return ret;
}

const DataPtr& BMemory::getOrNullShallow(int item) {
    int idx = find(item);
    if(idx==end) return DataPtr::NULLP;
    const auto& ret = rawContents[idx];
    if (ret.existsAndTypeEquals(FUTURE)) [[unlikely]] {
        auto prevRet = static_cast<Future*>(ret.get());
        auto resVal = prevRet->getResult();
        unsafeSet(item, resVal.get());
        attached_threads.erase(prevRet);
        prevRet->removeFromOwner();
        return rawContents[idx];
    }
    return ret;
}

const DataPtr& BMemory::getOrNull(int item, bool allowMutable) {
    int idx = find(item);
    if(idx==end) {
        if(parent) return parent->getOrNull(item, allowMutables && allowMutable);
        return DataPtr::NULLP;
    }
    const auto& ret = rawContents[idx];
    if (ret.existsAndTypeEquals(FUTURE)) [[unlikely]] {
        auto prevRet = static_cast<Future*>(ret.get());
        auto resVal = prevRet->getResult();
        unsafeSet(item, resVal.get());
        attached_threads.erase(prevRet);
        prevRet->removeFromOwner();
        return rawContents[idx];
    }
    if (ret.islitorexists()) [[likely]] {bbassert(allowMutable || ret.isA(), "Mutable symbol cannot be accessed from a nested block: " + variableManager.getSymbol(item));}
    else if (parent) return parent->getOrNull(item, allowMutables && allowMutable);
    return ret;
}

void BMemory::setToNullIgnoringFinals(int item) { 
    int idx = find(item);
    if(idx==end) return;
    auto& prev = rawContents[idx];
    if(prev.exists()) {
        Data* prevData = prev.get();
        if(prevData->getType()==ERRORTYPE && !static_cast<BError*>(prevData)->isConsumed()) bberror("Trying to overwrite an unhandled error:\n"+prevData->toString(this));
        prevData->removeFromOwner();
    }
    prev = DataPtr::NULLP;
}

void BMemory::prefetch() const {
    if(rawContents) _mm_prefetch(rawContents, _MM_HINT_T0);
}

void BMemory::set(int item, const DataPtr& value) {
    int idx = find(item);
    if(idx==end) {
        value.existsAddOwner();
        if(first_item==INT_MAX && item>=4) {
            first_item = item;
            rawContents[0] = value;
            rawContents[0].setAFalse();
            return;
        }
        data[item] = rawContentsSize;
        rawContents[rawContentsSize] = value;
        rawContents[rawContentsSize].setAFalse();
        ++rawContentsSize;
        if(rawContentsSize>=rawContentsReserved) [[unlikely]] {
            int newRawContentsReserved = rawContentsSize + rawContentsSize>>2; // + 25% growth (right shift is 1/4)
            rawContents = (DataPtr*)aligned_realloc(rawContents, rawContentsReserved, newRawContentsReserved, sizeof(DataPtr));
            rawContentsReserved = newRawContentsReserved;
        }
        return;
    }
    auto& prev = rawContents[idx];
    if(prev.isA()) bberror("Cannot overwrite final value: " + variableManager.getSymbol(item));
    if(prev.existsAndTypeEquals(ERRORTYPE) && !static_cast<BError*>(prev.get())->isConsumed()) bberror("Trying to overwrite an unhandled error:\n"+prev->toString(this));
    value.existsAddOwner();
    prev.existsRemoveFromOwner();
    prev = value;
    prev.setAFalse();
}

void BMemory::setFuture(int item, const DataPtr& value) {  // value.exists() == true always considering that this will be a valid future objhect
    int idx = find(item);
    if(idx==end) {
        value->addOwner();
        if(first_item==INT_MAX && item>=4) {
            first_item = item;
            rawContents[0] = DataPtr(value.get(), IS_FUTURE);
            rawContents[0].setAFalse();
            return;
        }
        data[item] = rawContentsSize;
        rawContents[rawContentsSize] = value;
        rawContents[rawContentsSize].setAFalse();
        ++rawContentsSize;
        if(rawContentsSize>=rawContentsReserved) [[unlikely]] {
            int newRawContentsReserved = rawContentsSize + rawContentsSize>>2; // + 25% growth (right shift is 1/4)
            rawContents = (DataPtr*)aligned_realloc(rawContents, rawContentsReserved, newRawContentsReserved, sizeof(DataPtr));
            rawContentsReserved = newRawContentsReserved;
        }
        return;
    }
    auto& prev = rawContents[idx];
    if(prev.isA()) bberror("Cannot overwrite final value: " + variableManager.getSymbol(item));
    if(prev.existsAndTypeEquals(ERRORTYPE) && !static_cast<BError*>(prev.get())->isConsumed()) bberror("Trying to overwrite an unhandled error:\n"+prev->toString(this));
    value->addOwner();
    prev.existsRemoveFromOwner();
    prev = DataPtr(value); //std::move(DataPtr(value.get(), IS_FUTURE));
    prev.setAFalse();
}

void BMemory::unsafeSet(int item, const DataPtr& value) {
    int idx = find(item);
    if(idx==end) {
        value.existsAddOwner();
        if(first_item==INT_MAX && item>=4) {
            first_item = item;
            rawContents[0] = value;
            rawContents[0].setAFalse();
            return;
        }
        data[item] = rawContentsSize;
        rawContents[rawContentsSize] = value;
        rawContents[rawContentsSize].setAFalse();
        ++rawContentsSize;
        if(rawContentsSize>=rawContentsReserved) [[unlikely]] {
            int newRawContentsReserved = rawContentsSize + rawContentsSize>>2; // + 25% growth (right shift is 1/4)
            rawContents = (DataPtr*)aligned_realloc(rawContents, rawContentsReserved, newRawContentsReserved, sizeof(DataPtr));
            rawContentsReserved = newRawContentsReserved;
        }
        return;
    }
    DataPtr& prev = rawContents[idx];
    bool prevFinal = prev.isA();
    if(prev.existsAndTypeEquals(ERRORTYPE) && !static_cast<BError*>(prev.get())->isConsumed()) bberror("Trying to overwrite an unhandled error:\n"+prev->toString(this));
    value.existsAddOwner();
    prev.existsRemoveFromOwner();

    /*f(value.existsAndTypeEquals(FUTURE)) prev = DataPtr(value.get(), IS_FUTURE);
    else*/ prev = value;
    prev.setA(prevFinal);
}

void BMemory::directTransfer(int to, int from) {
    int fromidx = find(from);
    auto value = (fromidx==end || !rawContents[fromidx].islitorexists()) ? (parent?parent->get(from, allowMutables):DataPtr::NULLP) : rawContents[fromidx];

    int toidx = find(to);
    if(toidx==end) {
        value.existsAddOwner();
        if(first_item==INT_MAX && to>=4) {
            first_item = to;
            rawContents[0] = value;
            rawContents[0].setAFalse();
            return;
        }
        data[to] = rawContentsSize;
        rawContents[rawContentsSize] = value;
        ++rawContentsSize;

        return;
    }
    DataPtr& prev = rawContents[toidx];
    if(prev.isA()) bberror("Cannot overwrite final value: " + variableManager.getSymbol(to));
    //if(prev.existsAndTypeEquals(ERRORTYPE) && !static_cast<BError*>(prev.get())->isConsumed()) bberror("Trying to overwrite an unhandled error:\n"+prev->toString(this));

    value.existsAddOwner();
    prev.existsRemoveFromOwner();

    prev = value;
    prev.setAFalse();
}

void BMemory::setFinal(int item) {
    await();
    int idx = find(item);
    bbassert(idx!=end, "Missing variable cannot be set to final: "+variableManager.getSymbol(item));
    DataPtr& value = rawContents[idx];
    bbassert(value.islitorexists(), "Missing variable cannot be set to final: "+variableManager.getSymbol(item));
    value.setA(true);
    hasAtLeastOneFinal = true;
}

void BMemory::pull(BMemory* other) {
    for(int idx=0;idx<other->max_cache_size;++idx) {
        const auto& dat = other->rawContents[idx];
        if (dat.islitorexists()) {
            //this can not be stored in the first cache element anyway
            int item = idx + other->first_item;
            set(item, dat);
        }
    }
    for(const auto& it : other->data) {
        const auto& dat = other->rawContents[it.second];
        if (dat.islitorexists() && it.first!=variableManager.thisId) set(it.first, dat);
    }
}

void BMemory::replaceMissing(BMemory* other) {
    for(int idx=0;idx<other->max_cache_size;++idx) {
        const auto& dat = other->rawContents[idx];
        if (dat.islitorexists()) {
            //this can not be stored in the first cache element anyway
            int item = idx + other->first_item;
            if(!getOrNullShallow(item).islitorexists()) set(item, dat);
        }
    }
    for(const auto& it : other->data) {
        const auto& dat = other->rawContents[it.second];
        if (dat.islitorexists() && !!getOrNullShallow(it.first).islitorexists()) set(it.first, dat);
    }
}

void BMemory::await() {
    //if(attached_threads.size()==0) return; // we don't need to lock because on zero threads we are ok, on >=1 threads we don't care about the number
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

    //for (const auto& dat : contents) {
    for(int i=0;i<rawContentsSize;++i) {
        const auto& dat = rawContents[i];
        if (dat.existsAndTypeEquals(ERRORTYPE) && !static_cast<BError*>(dat.get())->isConsumed())  {
            static_cast<BError*>(dat.get())->consume();
            destroyerr += "\033[0m(\x1B[31m ERROR \033[0m) The following error was but never handled:\n"+dat->toString(this)+"\n";
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