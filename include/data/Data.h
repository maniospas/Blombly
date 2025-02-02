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

#ifndef DATA_H
#define DATA_H

#include <memory>
#include <vector>
#include <string>
#include <iostream>
#include <atomic>
#include "common.h"
#include "Result.h"

class BMemory;
class Data {
public:
    inline Datatype getType() const { return type; }

    Data(Datatype type);
    virtual ~Data() = default;

    virtual size_t toHash() const;
    virtual bool isSame(const DataPtr& other);

    virtual Result push(BMemory* memory, const DataPtr& other) {bberror("Not implemented: push("+std::string(datatypeName[getType()])+")");}
    virtual Result pop(BMemory* memory) {bberror("Not implemented: pop("+std::string(datatypeName[getType()])+")");}
    virtual Result next(BMemory* memory) {bberror("Not implemented: next("+std::string(datatypeName[getType()])+")");}
    virtual Result at(BMemory* memory, const DataPtr& other) {bberror("Not implemented: at("+std::string(datatypeName[getType()])+", "+other.torepr()+")");}
    virtual Result put(BMemory* memory, const DataPtr& position, const DataPtr& value) {bberror("Not implemented: neq("+std::string(datatypeName[getType()])+", "+position.torepr()+", "+value.torepr()+")");}
    virtual void clear(BMemory* memory) {bberror("Not implemented: clear("+std::string(datatypeName[getType()])+")");}
    virtual int64_t len(BMemory* memory) {bberror("Not implemented: len("+std::string(datatypeName[getType()])+")");}
    virtual Result move(BMemory* memory) {bberror("Not implemented: move("+std::string(datatypeName[getType()])+")");}
    virtual Result iter(BMemory* memory) {bberror("Not implemented: iter("+std::string(datatypeName[getType()])+")");}
    virtual double toFloat(BMemory* memory) {bberror("Not implemented: float("+std::string(datatypeName[getType()])+")");}
    virtual bool toBool(BMemory* memory) {bberror("Not implemented: bool("+std::string(datatypeName[getType()])+")");}
    virtual int64_t toInt(BMemory* memory) {bberror("Not implemented: int("+std::string(datatypeName[getType()])+")");}
    virtual Result add(BMemory* memory, const DataPtr& other) {bberror("Not implemented: add("+std::string(datatypeName[getType()])+", "+other.torepr()+")");}
    virtual Result sub(BMemory* memory, const DataPtr& other) {bberror("Not implemented: sub("+std::string(datatypeName[getType()])+", "+other.torepr()+")");}
    virtual Result rsub(BMemory* memory, const DataPtr& other) {bberror("Not implemented: rsub("+std::string(datatypeName[getType()])+", "+other.torepr()+")");}
    virtual Result mul(BMemory* memory, const DataPtr& other) {bberror("Not implemented: mul("+std::string(datatypeName[getType()])+", "+other.torepr()+")");}
    virtual Result div(BMemory* memory, const DataPtr& other) {bberror("Not implemented: div("+std::string(datatypeName[getType()])+", "+other.torepr()+")");}
    virtual Result rdiv(BMemory* memory, const DataPtr& other) {bberror("Not implemented: rdiv("+std::string(datatypeName[getType()])+", "+other.torepr()+")");}
    virtual Result pow(BMemory* memory, const DataPtr& other) {bberror("Not implemented: pow("+std::string(datatypeName[getType()])+", "+other.torepr()+")");}
    virtual Result rpow(BMemory* memory, const DataPtr& other) {bberror("Not implemented: rpow("+std::string(datatypeName[getType()])+", "+other.torepr()+")");}
    virtual Result mmul(BMemory* memory, const DataPtr& other) {bberror("Not implemented: mmul("+std::string(datatypeName[getType()])+", "+other.torepr()+")");}
    virtual Result mod(BMemory* memory, const DataPtr& other) {bberror("Not implemented: mod("+std::string(datatypeName[getType()])+", "+other.torepr()+")");}
    virtual Result rmod(BMemory* memory, const DataPtr& other) {bberror("Not implemented: rmod("+std::string(datatypeName[getType()])+", "+other.torepr()+")");}
    virtual Result lt(BMemory* memory, const DataPtr& other) {bberror("Not implemented: lt("+std::string(datatypeName[getType()])+", "+other.torepr()+")");}
    virtual Result le(BMemory* memory, const DataPtr& other) {bberror("Not implemented: le("+std::string(datatypeName[getType()])+", "+other.torepr()+")");}
    virtual Result gt(BMemory* memory, const DataPtr& other) {bberror("Not implemented: gt("+std::string(datatypeName[getType()])+", "+other.torepr()+")");}
    virtual Result ge(BMemory* memory, const DataPtr& other) {bberror("Not implemented: ge("+std::string(datatypeName[getType()])+", "+other.torepr()+")");}
    virtual Result eq(BMemory* memory, const DataPtr& other) {bberror("Not implemented: eq("+std::string(datatypeName[getType()])+", "+other.torepr()+")");}
    virtual Result neq(BMemory* memory, const DataPtr& other) {bberror("Not implemented: neq("+std::string(datatypeName[getType()])+", "+other.torepr()+")");}
    virtual Result opand(BMemory* memory, const DataPtr& other) {bberror("Not implemented: and("+std::string(datatypeName[getType()])+", "+other.torepr()+")");}
    virtual Result opor(BMemory* memory, const DataPtr& other) {bberror("Not implemented: or("+std::string(datatypeName[getType()])+", "+other.torepr()+")");}
    virtual Result min(BMemory* memory) {bberror("Not implemented: min("+std::string(datatypeName[getType()])+")");}
    virtual Result max(BMemory* memory) {bberror("Not implemented: max("+std::string(datatypeName[getType()])+")");}
    virtual Result logarithm(BMemory* memory) {bberror("Not implemented: log("+std::string(datatypeName[getType()])+")");}
    virtual Result sum(BMemory* memory) {bberror("Not implemented: sum("+std::string(datatypeName[getType()])+")");}
    virtual Result opnot(BMemory* memory) {bberror("Not implemented: not("+std::string(datatypeName[getType()])+")");}

    virtual std::string toString(BMemory* memory)= 0;
    
    inline void addOwner() {++referenceCounter;}
    virtual void removeFromOwner() {if((--referenceCounter)==0) delete this;}
protected:
    std::atomic<int> referenceCounter;
private:
    Datatype type;
};

std::string DataPtr::torepr() const {
    if(datatype & IS_FLOAT) return std::to_string(unsafe_tofloat());
    if(datatype & IS_INT) return std::to_string(unsafe_toint());
    if(datatype & IS_BOOL) return unsafe_tobool()?"true":"false";
    if(datatype & IS_ERROR) return "error";
    if(datatype & IS_PTR) {
        Data* obj = get(); 
        if(!obj) return "missing"; 
        return datatypeName[obj->getType()];
    }//return "Data object at memory address: "+std::to_string(data);
    return "Corrupted data with internal pointer type: "+std::to_string(datatype);
}

bool DataPtr::existsAndTypeEquals(Datatype type) const {
    if(datatype & IS_NOT_PTR) return false;
    if(!data) return false;
    return std::bit_cast<Data*>(data)->getType() == type;
}

void DataPtr::existsAddOwner() const {
    if(datatype & IS_NOT_PTR) return;
    if(data) std::bit_cast<Data*>(data)->addOwner();
}

void DataPtr::existsRemoveFromOwner() const {
    if(datatype & IS_NOT_PTR) return;
    if(data) std::bit_cast<Data*>(data)->removeFromOwner();
}

bool DataPtr::isSame(const DataPtr& other) const {
    if((other.datatype & REMOVE_PROPERTIES)!=(datatype & REMOVE_PROPERTIES)) return false;
    if(datatype & IS_NOT_PTR) return data==other.data;
    if(!data) return !other.data;
    return get()->isSame(other);
}

#endif // DATA_H
