# Rejects converting a non-virtual diamond object to its duplicated common base.
class Root {
}

class Left : Root {
}

class Right : Root {
}

class Diamond : Left, Right {
}

start {
    Diamond diamond = Diamond()
    Root root = diamond
}
