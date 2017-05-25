#include <iostream>

void foo(void) {
	std::cout << "foo called\n";
}

void bar(void) {
	std::cout << "bar called\n";
}

class Example {
    void sayHello(void) {
        std::cout << "Hello" << std::endl;
    }
};
