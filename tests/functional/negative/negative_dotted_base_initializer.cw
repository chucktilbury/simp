# Dotted forms are no longer valid base-initializer syntax.
class Base {
    Base() {
    }
}

class Child : Base {
    Child() {
        super.Base()
    }
}

start {
}
