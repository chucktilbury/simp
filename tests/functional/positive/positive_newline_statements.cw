# Checks newline-delimited statements work without semicolons and comments preserve statement boundaries.
class Base {
    int value
    Base(int initial) {
        value = initial
    }
}

class Child : Base {
    Child(int initial) {
        super Base(initial)
    }

    int read() {
        return value
    }
}

start {
    ; Cwhip-style line comment
    Child child = Child(40)
    child.value = child.read() + 2
    # Hash line comment
    print(child.value)
    // C++-style line comment
    /* Block comment
       retaining its line boundary */
}
