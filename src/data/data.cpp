#include "data/Data.h"

Data::Data(Datatype type) : type(type), referenceCounter(0) {}
bool Data::isSame(const DataPtr& other) {return other==this;}
size_t Data::toHash() const {return (size_t)this;}