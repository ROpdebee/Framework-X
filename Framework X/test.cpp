#include <iostream>

bool someFunction() {
    return true;
}

bool someOtherFunction() {
    return false;
}

int main() {
    if (someFunction() == true) {
        if (someOtherFunction() != false) {
            return 0x1;
        } else return 2;
    } else if (someOtherFunction() == true) {
        return 3;
    } else return 4;
    
    return 0;
}
