#include <cassert>

class LargeData {};

void someFunction(LargeData *data) {
    assert(data != NULL);
    
    // Do things with data
    return;
}

void addToData(LargeData *data) {
    assert(data != NULL);
    
    // Do things to data
    int foo = 5;
    data + foo;
}

bool isDataValid(LargeData *data) {
    return data != NULL;
}
