import greeting as Parent

class Derived: Parent {
    Derived() {
        super Parent()
        print(Parent.answer())
    }
    int read() { return Parent.answer() }
}

start {
    Derived value = Derived()
    print(value.read())
}
