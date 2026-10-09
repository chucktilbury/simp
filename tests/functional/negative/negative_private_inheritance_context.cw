# Rejects accessing a privately inherited field from a further-derived class.
class Base {
    int value
}

class PrivateChild : private Base {
}

class Grandchild : public PrivateChild {
    int readValue() {
        return value
    }
}

start {
}
