# Rejects a derived constructor calling an inaccessible private base constructor.
class Base {
    private:
    Base() {
    }
}

class Child : public Base {
    Child() {
        super Base()
    }
}

start {
}
