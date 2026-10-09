# Rejects converting a privately derived object to its inaccessible base type.
class Base {
}

class PrivateChild : private Base {
}

start {
    PrivateChild child = PrivateChild()
    Base base = child
}
