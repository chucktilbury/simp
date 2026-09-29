# Defines a declared method out of line within its owning namespace and invokes it.
namespace Remote {
    class Method {
        int value()
    }

    int Method.value() {
        return 42
    }
}

start {
    print(Remote.Method().value())
}
