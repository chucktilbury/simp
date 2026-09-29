# Rejects an unqualified field lookup when two non-virtual base subobjects provide that field.
class Root {
    int value
}

class Left : Root {
}

class Right : Root {
}

class Diamond : Left, Right {
}

start {
    Diamond diamond = Diamond()
    print(diamond.value)
}
