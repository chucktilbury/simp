# Rejects converting a protectedly derived object to its inaccessible base type.
class Base {
}

class ProtectedChild : protected Base {
}

start {
    ProtectedChild child = ProtectedChild()
    Base base = child
}
