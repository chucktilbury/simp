# A class can initialize its virtual base directly; when used as a base,
# its initializer and argument expression are skipped.
class A {
    int value

    A(int initial) {
        value = initial
        print(format("A:{}", initial))
    }

    destroy {
        print(format("A-{}", value))
    }
}

class B : virtual A {
    int initializerArgument() {
        print("B initializer argument")
        return 1
    }

    B() {
        try {
            super virtual A(initializerArgument())
        } except() {
            raise()
        }
        print("B")
    }

    destroy {
        print("B-")
    }
}

class C : B {
    C() {
        super virtual A(2)
        super B()
        print("C")
    }

    destroy {
        print("C-")
    }
}

start {
    B direct = B()
    print(direct.A.value)
    C derived = C()
    print(derived.B.A.value)
    direct.destroy()
    derived.destroy()
}
