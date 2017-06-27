#include <iostream>

class A {
    int a;
    int b;
public:
    A(int _a, int _b) : a(_a), b(_b) {}
};

class B {
    const int a;
    bool _foo;
    int fooInt() { return static_cast<int>(_foo); }
public:
    B(int _a, bool foo) : a(_a), _foo(foo) {}
    B(const B &b) : a(b.a), _foo(b._foo) {}
    
    bool foo() { return _foo; }
    void foo(bool newFoo) { _foo = newFoo; }
    int get_a() { return a; }
    
    int performComputation() {
        return a + fooInt();
    }
};
