#include "data/Jitable.h"
#include "data/Iterator.h"
#include "data/Boolean.h"
#include "data/BError.h"
#include "BMemory.h"
#include "interpreter/Command.h"
#include "common.h"
#include <unordered_set>

extern BError* OUT_OF_RANGE;


// Class representing a Jitable that returns a primitive
class ReturnPrimitiveJitable : public Jitable {
private:
    Data* primitive;
public:
    explicit ReturnPrimitiveJitable(Data* primitive): primitive(primitive) {}
    virtual bool run(BMemory* memory, Data*& returnValue, bool &returnSignal) override {
        returnValue = primitive;
        returnSignal = true;
        return true;
    }
};


// Class representing a Jitable that returns a primitive
class NextAsExistsJitable : public Jitable {
private:
    int from;
    int next;
    int as;
    int exists;
    bool setNext;
    bool setExists;
public:
    explicit NextAsExistsJitable(int from, int next, int as, int exists, bool setNext, bool setExists): from(from), next(next), as(as), exists(exists), setNext(setNext), setExists(setExists) {}
    virtual bool run(BMemory* memory, Data*& returnValue, bool &returnSignal) override {
        auto iterator = memory->get(from);
        if(iterator->getType()!=ITERATOR) // optimize only for iterators
            return false;
        auto it = static_cast<Iterator*>(iterator);
        Data* nextValue = it->fastNext();
        //if(!nextValue) return false;
        if(!nextValue) {
            BuiltinArgs args;
            args.size = 1;
            args.arg0 = it;
            Result res = it->implement(NEXT, &args);
            nextValue = res.get();
            
            if(setNext) memory->unsafeSet(next, nextValue, nullptr);
            memory->unsafeSet(as, nextValue, nullptr);
            Data* ret = (nextValue==OUT_OF_RANGE)?Boolean::valueFalse:Boolean::valueTrue;
            if(setExists) memory->unsafeSet(exists, ret, nullptr);
            returnValue = ret;
            return true;
        }
        if(setNext) memory->unsafeSet(next, nextValue, nullptr);
        memory->unsafeSet(as, nextValue, nullptr);
        Data* ret = (nextValue==OUT_OF_RANGE)?Boolean::valueFalse:Boolean::valueTrue;
        if(setExists) memory->unsafeSet(exists, ret, nullptr);
        returnValue = ret;
        return true;
    }
};

// Implementation of the jit function
Jitable* jit(const Code* code) {
    std::vector<Command*>* program = code->getProgram();
    int start = code->getStart();
    int end = code->getEnd();
    int size = end-start;
    if(size==2) {
        Command* c0 = program->at(start);
        Command* c1 = program->at(start+1);
        if(c0->operation==BUILTIN && c1->operation==RETURN && c0->args[0]==c1->args[1]) 
            return new ReturnPrimitiveJitable(c0->value);
    }
    
    if(size==3) {
        Command* c0 = program->at(start);
        Command* c1 = program->at(start+1);
        Command* c2 = program->at(start+2);
        if(c0->operation==NEXT && c1->operation==AS && c2->operation==EXISTS)
        {
            /*std::cout << "jit\n";
            std::cout << variableManager.getSymbol(c0->args[0])<<" "<<variableManager.getSymbol(c0->args[1])<<"\n";
            std::cout << variableManager.getSymbol(c1->args[0])<<" "<<variableManager.getSymbol(c1->args[1])<<"\n";
            std::cout << variableManager.getSymbol(c2->args[0])<<" "<<variableManager.getSymbol(c2->args[1])<<"\n";*/
            if(c0->args[0]==c1->args[1] && c1->args[0]==c2->args[1])  
                return new NextAsExistsJitable(c0->args[1], c0->args[0], c1->args[0], c2->args[0], !c0->knownLocal[0], !c2->knownLocal[0]);
        }
    }

    /*std::unordered_set<int> inputs;
    std::unordered_set<int> outputs;

    for(int i=start;i<=end;++i) {

    }*/
    return nullptr;
}
