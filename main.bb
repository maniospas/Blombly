#include "libs/err"

test = {
    return err::invalid("failed");
}
x = test();
print("test "+str(x));
print("here");