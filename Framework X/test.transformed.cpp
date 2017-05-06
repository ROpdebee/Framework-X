#include <iostream>

bool someFunction() {
    return true;
}

bool someOtherFunction() {
    return false;
}

int main() {
    // Replaced!
if (someFunction()) {
        // Replaced!
if (someOtherFunction()) {
            return 1;
        } else return 2;
    } else // Replaced!
if (someOtherFunction()) {
        return 3;
    } else {
        return 4;
    }
    
    return 0;
}
