/*
   Copyright 2024 Emmanouil Krasanakis

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
 */

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


std::string indentNewlines(const std::string& input) {
    std::string output("  \033[34m|\033[0m");
    output.reserve(input.size() * 3/2);
    
    for (size_t i = 0; i < input.size(); ++i) {
        output += input[i];
        if (input[i] == '\n' && i + 1 != input.size()) { // Avoid adding at the very end
            output += "  \033[34m|\033[0m";
        }
    }
    return output;
}

std::atomic<unsigned long long> countUnrealeasedMemories(0);
extern VariableManager variableManager;

void BMemory::verify_noleaks() {
    //bbassert(memories.size() == 0, "There are " + std::to_string(memories.size()) + " leftover memory contexts leaked");
    int countUnreleased = countUnrealeasedMemories.load();
    countUnrealeasedMemories = 1;  // shared is always leftover
    bbassert(countUnreleased == 1, "There are " + std::to_string(countUnreleased-1) + " leftover memory contexts leaked");  // the main memory is a global object (needed to sync threads on errors)
}

BMemory::BMemory(unsigned int depth, BMemory* par, int expectedAssignments) : depth(depth), parent(par), allowMutables(true), first_item(INT_MAX), hasAtLeastOneFinal(false) { 
    ++countUnrealeasedMemories;
    cache_size = expectedAssignments;
    cache = new DataPtr[cache_size]();
    //for(int i=0;i<cache_size;++i) cache[i] = DataPtr::NULLP;  // TODO: find a way to reduce these operations?
}

void BMemory::release() {
    std::string destroyerr = "";
    for(const auto& thread_ : attached_threads) {
        Future* thread = static_cast<Future*>(thread_.get().get());
        try {Result res = thread->getResult();}
        catch (const BBError& e) {destroyerr += std::string(e.what())+"\n";}
    }
    
    attached_threads.clear();
    try {runFinally();}
    catch(const BBError& e) {destroyerr += std::string(e.what())+"\n";}

    for(const auto& thread_ : attached_threads) {
        Future* thread = static_cast<Future*>(thread_.get().get());
        try {thread->getResult();}
        catch (const BBError& e) {destroyerr += std::string(e.what())+"\n";}
    }
    attached_threads.clear();

    //for(const auto& dat : contents) {
    for(unsigned int i=0;i<cache_size;++i) {
        auto& dat = cache[i];
        try {dat.existsRemoveFromOwner();}
        catch(const BBError& e) {destroyerr += std::string(e.what())+"\n";}
    }
    for(auto& dat_ : data) {
        auto dat = dat_.second;
        try {dat.existsRemoveFromOwner();}
        catch(const BBError& e) {destroyerr += std::string(e.what())+"\n";}
    }
    data.clear();
    cache_size = 0;
    if(cache_size) delete[] cache;
    if(destroyerr.size()) throw BBError(destroyerr.substr(0, destroyerr.size()-1));
}

BMemory::~BMemory() {
    countUnrealeasedMemories--; 
    release();
}

int BMemory::size() const {return data.size();}

void BMemory::resolveFuture(DataPtr& ret) {
    Future* prevRet = static_cast<Future*>(ret.get());
    Result resVal = prevRet->getResult();
    DataPtr value = resVal.get();
    value.existsAddOwner();

    bool prevFinal = ret.isA();
    value.setA(prevFinal);
    ret = value;
    prevRet->removeFromOwner();
}


const DataPtr& BMemory::get(int item, bool allowMutable) {
    auto& ret = find(item);
    if(ret.existsAndTypeEquals(FUTURE)) [[unlikely]] {
        resolveFuture(ret);
        return get(item, allowMutable);
    }
    if(ret.islitorexists()) [[likely]] {bbassert(allowMutable || ret.isA(), "Non-final symbol found but cannot be accessed from another scope: " + variableManager.getSymbol(item));}
    else if(parent) return parent->get(item, allowMutables && allowMutable);
    else bberror("Missing value: " + variableManager.getSymbol(item));
    return ret;
}

const DataPtr& BMemory::getShallow(int item) {
    auto& ret = find(item);
    if(ret.islitorexists()) return ret;
    bberror("Missing value: " + variableManager.getSymbol(item));
}

const DataPtr& BMemory::getOrNullShallow(int item) {
    return find(item);
}

const DataPtr& BMemory::getOrNull(int item, bool allowMutable) {
    auto& ret = find(item);
    if (ret.islitorexists()) [[likely]] {bbassert(allowMutable || ret.isA(), "Mutable symbol cannot be accessed from a nested block: " + variableManager.getSymbol(item));}
    else if (parent) return parent->getOrNull(item, allowMutables && allowMutable);
    return ret;
}

void BMemory::setToNullIgnoringFinals(int item) {
    auto& prev = find(item);
    /*if(prev.exists()) {
        Data* prevData = prev.get();
        //if(prevData->getType()==ERRORTYPE && !static_cast<BError*>(prevData)->isConsumed()) bberror("Trying to overwrite an unhandled error:\n"+prevData->toString(this));
        prevData->removeFromOwner();
    }*/
    prev.existsRemoveFromOwner();
    prev = DataPtr::NULLP;
}

void BMemory::set(int item, const DataPtr& value) {
    DataPtr& prev = find(item);
    if(prev.isA()) bberror("Cannot overwrite final value: " + variableManager.getSymbol(item));
    if(prev.existsAndTypeEquals(ERRORTYPE) && !static_cast<BError*>(prev.get())->isConsumed()) 
        throw BBError(
                static_cast<BError*>(prev.get())->consume()->toString(this)+
                "\n \033[33m !!! \033[0mAt this point, the error is returned because it was never caught"
                "\n      but was going to be hidden due to overwriting it with a new value\n      by the next instruction in the trace.");
    value.existsAddOwner();
    prev.existsRemoveFromOwner();
    prev = value;
    prev.setAFalse();
}

void BMemory::setFuture(int item, const DataPtr& value) {  // value.exists() == true always considering that this will be a valid future objhect
    auto& prev = find(item);
    if(prev.isA()) bberror("Cannot overwrite final value: " + variableManager.getSymbol(item));
    if(prev.existsAndTypeEquals(ERRORTYPE) && !static_cast<BError*>(prev.get())->isConsumed()) 
        throw BBError(
                static_cast<BError*>(prev.get())->consume()->toString(this)+
                "\n \033[33m !!! \033[0mAt this point, the error is returned because it was never caught"
                "\n      but was going to be hidden due to overwriting it with a new value\n      by the next instruction in the trace.");
    value->addOwner();
    prev.existsRemoveFromOwner();
    prev = DataPtr(value); //std::move(DataPtr(value.get(), IS_FUTURE));
    prev.setAFalse();
}

void BMemory::unsafeSet(int item, const DataPtr& value) {
    auto& prev = find(item);
    bool prevFinal = prev.isA();
    if(prev.existsAndTypeEquals(ERRORTYPE) && !static_cast<BError*>(prev.get())->isConsumed()) 
        throw BBError(
                static_cast<BError*>(prev.get())->consume()->toString(this)+
                "\n \033[33m !!! \033[0mAt this point, the error is returned because it was never caught"
                "\n      but was going to be hidden due to overwriting it with a new value\n      by the next instruction in the trace.");
    value.existsAddOwner();
    prev.existsRemoveFromOwner();

    /*f(value.existsAndTypeEquals(FUTURE)) prev = DataPtr(value.get(), IS_FUTURE);
    else*/ prev = value;
    prev.setA(prevFinal);
}

void BMemory::directTransfer(Struct* to) {
    await();

    for(int idx=0;idx<cache_size;++idx) {
        auto& dat = cache[idx];
        if (dat.islitorexists()) {
            int item = idx + first_item;
            if(variableManager.getIdRetain(item)) {
                to->transferNoChecks(item, dat);
                dat = DataPtr::NULLP;
            }
        }
    }
    for(auto& it : data) {
        auto& dat = it.second;
        if (dat.islitorexists()) {
            if(variableManager.getIdRetain(it.first)) {
                to->transferNoChecks(it.first, dat);
                dat.existsAddOwner();
            }
        }
    }
}

void BMemory::directTransfer(int to, int from) {
    const auto& value = get(from);
    auto& prev = find(to);
    if(prev.isA()) bberror("Cannot overwrite final value: " + variableManager.getSymbol(to));
    if(prev.existsAndTypeEquals(ERRORTYPE) && !static_cast<BError*>(prev.get())->isConsumed()) 
        throw BBError(
                static_cast<BError*>(prev.get())->consume()->toString(this)+
                "\n \033[33m !!! \033[0mAt this point, the error is returned because it was never caught"
                "\n      but was going to be hidden due to overwriting it with a new value\n      by the next instruction in the trace.");
    
    value.existsAddOwner();
    /*if(prev.exists()) {
        if(prev->getType()==FUTURE) {static_cast<Future*>(prev.get())->getResult();}
        prev->removeFromOwner();
    }*/
    prev.existsRemoveFromOwner();

    prev = value;
    prev.setAFalse();
}

void BMemory::setFinal(int item) {
    //await();
    auto& value = find(item);
    bbassert(value.islitorexists(), "Missing variable cannot be set to final: "+variableManager.getSymbol(item));
    value.setA(true);
    hasAtLeastOneFinal = true;
}

void BMemory::pull(BMemory* other) {
    for(int idx=0;idx<other->cache_size;++idx) {
        const auto& dat = other->cache[idx];
        if (dat.islitorexists()) {
            int item = idx + other->first_item;
            set(item, dat);
        }
    }
    for(const auto& it : other->data) {
        const auto& dat = it.second;
        if (dat.islitorexists()) set(it.first, dat);
    }
}

void BMemory::replaceMissing(BMemory* other) {
    for(unsigned int idx=0;idx<cache_size;++idx) {
        const auto& dat = other->cache[idx];
        if (dat.islitorexists()) {
            //this can not be stored in the first cache element anyway
            int item = idx + other->first_item;
            if(!getOrNullShallow(item).islitorexists()) set(item, dat);
        }
    }
    for(const auto& it : other->data) {
        const auto& dat = it.second;
        if (dat.islitorexists() && !!getOrNullShallow(it.first).islitorexists()) set(it.first, dat);
    }
}


void BMemory::tempawait() {
    std::string destroyerr = "";
    int counterr = 0;
    for (const auto& thread_ : attached_threads) {
        Future* thread = static_cast<Future*>(thread_.get().get());
        try {thread->getResult();}
        catch (const BBError& e) {
            destroyerr += "\n";
            destroyerr += e.what();
            counterr++;
        }
    }
    attached_threads.clear();
    if(destroyerr.size()) throw BBError(counterr==1?destroyerr.substr(1):indentNewlines(destroyerr.substr(1)));
}


void BMemory::consumeAllErrors() {
    for (const auto& thread_ : attached_threads) {
        Future* thread = static_cast<Future*>(thread_.get().get());
        try {thread->getResult();}
        catch (const BBError& e) {}
    }
    attached_threads.clear();

    for(unsigned int i=0;i<cache_size;++i) {
        const auto& dat = cache[i];
        if (dat.existsAndTypeEquals(ERRORTYPE)) static_cast<BError*>(dat.get())->consume();
    }
    for (const auto& dat_ : data) {
        const auto& dat = dat_.second;
        if (dat.existsAndTypeEquals(ERRORTYPE))  static_cast<BError*>(dat.get())->consume();
    }
}

void BMemory::await() {
    //if(attached_threads.size()==0) return; // we don't need to lock because on zero threads we are ok, on >=1 threads we don't care about the number
    std::string destroyerr = "";
    int counterr = 0;
    for (const auto& thread_ : attached_threads) {
        Future* thread = static_cast<Future*>(thread_.get().get());
        try {thread->getResult();}
        catch (const BBError& e) {
            destroyerr += "\n";
            destroyerr += e.what();
            counterr++;
        }
    }
    attached_threads.clear();

    try {runFinally();}
    catch (const BBError& e) {
        destroyerr += "\n";
        destroyerr += e.what();
        counterr++;
    }

    for (const auto& thread_ : attached_threads) {
        Future* thread = static_cast<Future*>(thread_.get().get());
        try {thread->getResult();}
        catch (const BBError& e) {
            destroyerr += "\n";
            destroyerr += e.what();
            counterr++;
        }
    }
    attached_threads.clear();

    for(unsigned int i=0;i<cache_size;++i) {
        const auto& dat = cache[i];
        if (dat.existsAndTypeEquals(ERRORTYPE) && !static_cast<BError*>(dat.get())->isConsumed())  {
            static_cast<BError*>(dat.get())->consume();
            destroyerr += "\n"+dat->toString(this);
            counterr++;
        }
    }
    for (const auto& dat_ : data) {
        const auto& dat = dat_.second;
        if (dat.existsAndTypeEquals(ERRORTYPE) && !static_cast<BError*>(dat.get())->isConsumed())  {
            static_cast<BError*>(dat.get())->consume();
            destroyerr += "\n"+dat->toString(this);
            counterr++;
        }
    }
    if(destroyerr.size()) throw BBError(counterr==1?destroyerr.substr(1):indentNewlines(destroyerr.substr(1)));
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
            ExecutionInstance executor(depth, code, this, true); // everything deferred runs after all threads have been synchronized, so stay in thread (last true argument)
            auto returnedValue = executor.run(code);
            if(returnedValue.get().existsAndTypeEquals(ERRORTYPE)) bberror(returnedValue.get().get()->toString(nullptr));
            bbassert(!returnedValue.returnSignal, "Cannot return a value from within `defer`");
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