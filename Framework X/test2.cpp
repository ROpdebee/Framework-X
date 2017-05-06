class Test {
    int a;
    bool performAddition;
    
    Test(int number) : a(number) {}
    
    int add(int b) {
        if (performAddition == true) {
            a += b;
            return a;
        } else {
            return a;
        }
    }
};
