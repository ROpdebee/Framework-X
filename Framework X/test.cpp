#include <iostream>

bool someFunction() {
    return true;
}

bool someOtherFunction() {
    return false;
}

int main() {
    if (someFunction() == true) {
        if (someOtherFunction() != false) return 0x1;
        else return 2;
    } else if (someOtherFunction() == true) {
        return 3;
    } else return 0b100;
    
    return 0;
}

class Test {
private:
    int a;
    int b;
public:
    Test(int _a, int _b) : a(_a), b(_b) {}
};