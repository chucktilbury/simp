# Rejects field lookup when virtual and non-virtual paths produce ambiguous ancestor subobjects.
class Ancestor {
    int value
}

class VirtualPath : virtual Ancestor {
}

class OrdinaryPath : Ancestor {
}

class Diamond : VirtualPath, OrdinaryPath {
}

start {
    Diamond diamond = Diamond()
    print(diamond.value)
}
