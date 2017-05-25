class MyClass {
    int a;
    bool performAddition;
    int private_value;

public:
    MyClass(int number) : a(number) {}
    
    int add(int b) {
        if (performAddition == true) {
            a += b;
            return a;
        } else {
            return a;
        }
    }
};
