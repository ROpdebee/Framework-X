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
if (someOtherFunction()) return 0x1; else return true;
    } else // Replaced!
if (someOtherFunction()) {
        return 3;
    } else return 0b100;
    
    return 0;
}
