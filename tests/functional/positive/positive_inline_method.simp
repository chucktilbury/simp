# Uses an inline C block inside a method to update a captured parameter before returning it.
class Calculator {
    int adjust(int value) {
        inline (int value) {
            *value += 2;
        }
        return value
    }
}

start {
    print(Calculator().adjust(40))
}
