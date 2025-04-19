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

#include "data/BString.h"
#include "data/BFile.h"
#include "data/BError.h"
#include "data/Iterator.h"
#include "data/Database.h"
#include "data/Graphics.h"
#include "common.h"
#include <openssl/evp.h>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

extern BError* OUT_OF_RANGE;

inline std::string calculateHash(const std::string& input, const EVP_MD* (*hash_function)()) {
    EVP_MD_CTX* context = EVP_MD_CTX_new();
    bbassert(context, "OpenSSL error: Failed to create EVP_MD_CTX");
    unsigned char hash_result[EVP_MAX_MD_SIZE];
    unsigned int hash_length = 0;
    if (EVP_DigestInit_ex(context, hash_function(), nullptr) != 1 ||
        EVP_DigestUpdate(context, input.c_str(), input.size()) != 1 ||
        EVP_DigestFinal_ex(context, hash_result, &hash_length) != 1) {
        EVP_MD_CTX_free(context);
        bberror("OpenSSL error: Failed to compute hash");
    }
    EVP_MD_CTX_free(context);
    std::ostringstream hex_stream;
    for (unsigned int i = 0; i < hash_length; ++i) hex_stream << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash_result[i]);
    return hex_stream.str();
}


BString::BString(const std::string& val) : Data(STRING), contents(val) {}
BString::BString() : Data(STRING), contents("") {}
size_t BString::toHash() const {return std::hash<std::string>{}(contents);}
std::string BString::toString(BMemory* memory){return contents;}
std::string& BString::toString(){return contents;}

bool BString::isSame(const DataPtr& other) {
    if (other.existsAndTypeEquals(STRING)) return toString() == static_cast<BString*>(other.get())->toString();
    return false;
}

Result BString::eq(BMemory* memory, const DataPtr& other) {
    bbassert(other.existsAndTypeEquals(STRING), "Strings can only be compared to strings and not " + other.torepr());
    return RESMOVE(Result(toString() == static_cast<BString*>(other.get())->toString()));
}

Result BString::neq(BMemory* memory, const DataPtr& other) {
    bbassert(other.existsAndTypeEquals(STRING), "Strings can only be compared to strings and not " + other.torepr());
    return RESMOVE(Result(toString() != static_cast<BString*>(other.get())->toString()));
}

Result BString::lt(BMemory* memory, const DataPtr& other) {
    bbassert(other.existsAndTypeEquals(STRING), "Strings can only be compared to strings and not " + other.torepr());
    return RESMOVE(Result(toString() < static_cast<BString*>(other.get())->toString()));
}

Result BString::le(BMemory* memory, const DataPtr& other) {
    bbassert(other.existsAndTypeEquals(STRING), "Strings can only be compared to strings and not " + other.torepr());
    return RESMOVE(Result(toString() <= static_cast<BString*>(other.get())->toString()));
}

Result BString::gt(BMemory* memory, const DataPtr& other) {
    bbassert(other.existsAndTypeEquals(STRING), "Strings can only be compared to strings and not " + other.torepr());
    return RESMOVE(Result(toString() > static_cast<BString*>(other.get())->toString()));
}

Result BString::ge(BMemory* memory, const DataPtr& other) {
    bbassert(other.existsAndTypeEquals(STRING), "Strings can only be compared to strings and not " + other.torepr());
    return RESMOVE(Result(toString() >= static_cast<BString*>(other.get())->toString()));
}


Result BString::at(BMemory *memory, const DataPtr& other) {
    if(other.isint()) {
        int64_t index = other.unsafe_toint();
        int64_t n = (int64_t)contents.size();
        if (index < 0 || index >= n) return RESMOVE(Result(OUT_OF_RANGE));
        return RESMOVE(Result(new BString(std::string(1, contents[index]))));
    }
    if(other.existsAndTypeEquals(STRING)) {
        std::string v1 = toString(nullptr);
        std::string v2 = static_cast<BString*>(other.get())->toString(nullptr);

        bbassert(v2 == "md5" || v2 == "sha1" || v2 == "sha256" || v2 == "sha512" ||
                    v2 == "sha224" || v2 == "sha384" || v2 == "sha3_224" || v2 == "sha3_256" || 
                    v2 == "sha3_384" || v2 == "sha3_512" || v2 == "blake2b" || v2 == "blake2s" || 
                    v2 == "ripemd160" || v2 == "whirlpool" || v2 == "sm3", 
                    "Only md5, sha1, sha224, sha256, sha384, sha512, sha3_224, sha3_256, sha3_384, sha3_512, blake2b, blake2s, ripemd160, whirlpool, or sm3 formatting is allowed for strings");
            
        if (v2 == "sha1") return RESMOVE(Result(new BString(calculateHash(v1, EVP_sha1))));
        if (v2 == "sha224") return RESMOVE(Result(new BString(calculateHash(v1, EVP_sha224))));
        if (v2 == "sha256") return RESMOVE(Result(new BString(calculateHash(v1, EVP_sha256))));
        if (v2 == "sha384") return RESMOVE(Result(new BString(calculateHash(v1, EVP_sha384))));
        if (v2 == "sha512") return RESMOVE(Result(new BString(calculateHash(v1, EVP_sha512))));
        if (v2 == "sha3_224") return RESMOVE(Result(new BString(calculateHash(v1, EVP_sha3_224))));
        if (v2 == "sha3_256") return RESMOVE(Result(new BString(calculateHash(v1, EVP_sha3_256))));
        if (v2 == "sha3_384") return RESMOVE(Result(new BString(calculateHash(v1, EVP_sha3_384))));
        if (v2 == "sha3_512") return RESMOVE(Result(new BString(calculateHash(v1, EVP_sha3_512))));
        if (v2 == "blake2b") return RESMOVE(Result(new BString(calculateHash(v1, EVP_blake2b512))));
        if (v2 == "blake2s") return RESMOVE(Result(new BString(calculateHash(v1, EVP_blake2s256))));
        if (v2 == "ripemd160") return RESMOVE(Result(new BString(calculateHash(v1, EVP_ripemd160))));
        if (v2 == "whirlpool") return RESMOVE(Result(new BString(calculateHash(v1, EVP_whirlpool))));
        if (v2 == "sm3") return RESMOVE(Result(new BString(calculateHash(v1, EVP_sm3))));
        
        return RESMOVE(Result(new BString(calculateHash(v1, EVP_md5))));
    }

    if(other.exists()) {
        auto res = other->iter(memory);
        DataPtr _iterator = res.get();
        bbassert(_iterator.existsAndTypeEquals(ITERATOR), "String index is neither an integer nor can be converted to an iterator viat `iter`: "+other.torepr());
        Iterator* iterator = static_cast<Iterator*>(_iterator.get());

        // Treat contiguous iterators more efficiently
        if(iterator->isContiguous()) {
            int64_t start = iterator->getStart();
            int64_t end = iterator->getEnd();
            int64_t n = (int64_t)contents.size();
            if (start < 0 || start >= n || end < 0 || end > n|| start > end) return RESMOVE(Result(OUT_OF_RANGE));
            std::string result = contents.substr(start, end - start);
            return RESMOVE(Result(new BString(std::move(result))));
        } 
        else {
            // Handle non-contiguous iterators
            std::string result;
            result.reserve(iterator->expectedSize());
            while(true) {
                Result nextResult = iterator->next(memory);
                DataPtr indexData = nextResult.get();
                if (indexData == OUT_OF_RANGE) break; 
                bbassert(indexData.isint(), "String index iterator must contain integers: "+other.torepr());
                size_t index = (size_t)indexData.unsafe_toint();
                if (index >= contents.size()) return RESMOVE(Result(OUT_OF_RANGE));
                result += contents[index];
            }
            return RESMOVE(Result(new BString(result)));
        }
    }

    bberror("Strings only accept integer, string, or iterable indexes.");
}

int64_t BString::toInt(BMemory *memory) {
    char* endptr = nullptr;
    int64_t ret = std::strtol(contents.c_str(), &endptr, 10);
    if(endptr == contents.c_str() || *endptr != '\0') bberror("Failed to convert string to int");
    return ret;
}

double BString::toFloat(BMemory *memory) {
    char* endptr = nullptr;
    double ret = std::strtod(contents.c_str(), &endptr);
    if(endptr == contents.c_str() || *endptr != '\0') bberror("Failed to convert string to float");
    return ret;
}

bool BString::toBool(BMemory *memory) {
    if(contents=="true") return true;
    if(contents=="false") return false;
    bberror("Failed to convert string to bool");
}

Result BString::iter(BMemory *memory) {return RESMOVE(Result(new AccessIterator(this, contents.size())));}
Result BString::add(BMemory *memory, const DataPtr& other) {
    if(!other.existsAndTypeEquals(STRING)) {
        if(other.existsAndTypeEquals(ERRORTYPE)) return RESMOVE(Result(new BString(toString(nullptr)+other->toString(nullptr))));
        //if(other.existsAndTypeEquals(ERRORTYPE)) bberror(other->toString(nullptr));
        bberror("Strings can only be concatenated with strings");
    }

    BString* otherString = static_cast<BString*>(other.get());
    BString* ret = new BString(toString() + otherString->toString());
    /*ret->size = size + otherString->size;

    int retDepth;
    {
        std::lock_guard<std::recursive_mutex> lock(otherString->memoryLock);
        retDepth = otherString->depth;
    }
    {
        std::lock_guard<std::recursive_mutex> lock(memoryLock);
        if(depth<retDepth) depth = retDepth;
    }
    if(retDepth>512) {
        ret->front = ret->toString() + otherString->toString();
    }
    else {
        ret->depth = retDepth+1;
        ret->concatenated.push_back(this);
        ret->concatenated.push_back(otherString);
        ret->addOwner();
        otherString->addOwner();
    }*/

    return RESMOVE(Result(ret));
}
int64_t BString::len(BMemory *memory) {return contents.size();}