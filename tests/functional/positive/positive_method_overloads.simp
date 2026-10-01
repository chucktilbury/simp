# Exercises method overloading: several same-named methods distinguished by
# parameter type, resolved by exact argument-type match and dispatched
# through type-encoded mangled symbols. Also checks that an overload set
# survives inheritance -- a derived class may override one overload while
# inheriting its siblings, and may add a new overload of its own -- and that
# all of them dispatch correctly through a base-typed reference.
class Formatter {
    Formatter() {}

    int describe() {
        return 1
    }

    int describe(int value) {
        return value + 10
    }

    int describe(float value) {
        return 2
    }

    int describe(strg value) {
        return 3
    }

    int describe(int first, int second) {
        return first + second
    }
}

class LoudFormatter : Formatter {
    LoudFormatter() {
        super Formatter()
    }

    # Overrides exactly one member of the inherited overload set; the
    # zero-argument, float, string, and two-int overloads are inherited.
    int describe(int value) {
        return value + 100
    }

    # Adds a brand new overload that the base class does not declare.
    int describe(bool value) {
        return 55
    }
}

start {
    Formatter plain = Formatter()
    print(plain.describe())
    print(plain.describe(5))
    print(plain.describe(1.5))
    print(plain.describe("text"))
    print(plain.describe(3, 4))

    LoudFormatter loud = LoudFormatter()
    print(loud.describe())
    print(loud.describe(5))
    print(loud.describe(1.5))
    print(loud.describe(true))
    print(loud.describe(3, 4))

    # Virtual dispatch through a base-typed reference must still reach the
    # derived override for the int overload, and the inherited
    # implementations for the others.
    Formatter view = loud
    print(view.describe())
    print(view.describe(5))
    print(view.describe(3, 4))
}
