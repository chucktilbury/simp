# Rejects an override that changes the inherited method return type.
class Base {
    Base() {}

    int value(int input) {
        return input
    }
}

class Child : Base {
    Child() {
        super Base()
    }

    strg value(int input) {
        return "wrong"
    }
}

start {}
