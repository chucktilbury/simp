# Rejects an unqualified method call when two bases declare the same method.
class First {
    int value() {
        return 1
    }
}

class Second {
    int value() {
        return 2
    }
}

class Joined : First, Second {
}

start {
    Joined joined = Joined()
    print(joined.value())
}
