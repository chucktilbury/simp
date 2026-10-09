# The class being constructed directly must initialize a non-defaultable
# virtual base even though its intermediate base supplies an initializer.
class A {
    A(int value) {
    }
}

class B : virtual A {
    B() {
        super virtual A(1)
    }
}

class C : B {
    C() {
        super B()
    }
}

start {
    C value = C()
}
