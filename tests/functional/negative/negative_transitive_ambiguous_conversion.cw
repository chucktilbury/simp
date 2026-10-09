# Rejects conversion to an ancestor reached through both virtual and non-virtual paths.
class Ancestor {
}

class VirtualPath : virtual Ancestor {
}

class OrdinaryPath : Ancestor {
}

class Diamond : VirtualPath, OrdinaryPath {
}

start {
    Diamond diamond = Diamond()
    Ancestor ancestor = diamond
}
