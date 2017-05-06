class Test {
    int a;
    bool performAddition;
    
    Test(int number) : a(number) {}
    
    int add(int b) {
        // Replaced!
if (performAddition) {
            a += b;
            return a;
        } else {
            return a;
        }
    }
};
