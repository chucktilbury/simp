# Rejects defining a top-level class method from inside a namespace.
class Foo {
    int compute()
}

namespace Other {
    int Foo.compute() {
        return 99
    }
}

start {
    print(Foo().compute())
}
