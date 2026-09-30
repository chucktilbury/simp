# Each overload binds to its own out-of-line definition: the declaration,
# the definition, and the emitted symbol are all keyed by parameter types,
# not just the method name.
class Calc {
    int scale(int v)
    int scale(int v, int by)
}

int Calc.scale(int v) {
    return v * 2
}

int Calc.scale(int v, int by) {
    return v * by
}

start {
    Calc c = Calc()
    print(c.scale(5))
    print(c.scale(5, 3))
}
