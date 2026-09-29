# Checks parenthesized return expressions and an empty return in an out-of-line void method.
class Foo {
    int compute(int x)
    void announce()
}

int Foo.compute(int x) {
    return (x * 2)
}

void Foo.announce() {
    return ()
}

start {
    Foo foo = Foo()
    print(foo.compute(5))
    foo.announce()
    print(1)
}
