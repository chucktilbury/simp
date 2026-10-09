# Rejects defining a namespaced class method from a different namespace scope.
namespace A {
    class Foo {
        int compute()
    }
}

namespace B {
    int A.Foo.compute() {
        return 99
    }
}

start {
    print(A.Foo().compute())
}
