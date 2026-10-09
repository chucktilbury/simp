# Exercises the optional 'u' suffix: an unsuffixed integer literal is a
# valid unsigned literal wherever the context already expects `unsigned`.
class Counter {
    unsigned value

    Counter(unsigned initial) {
        value = initial
    }

    unsigned get() {
        return 7
    }
}

start {
    unsigned declared = 5
    print(declared)

    unsigned assigned = 0u
    assigned = 9
    print(assigned)

    unsigned large = 5000000000
    print(large)

    Counter counter = Counter(3)
    print(counter.value)
    print(counter.get())

    int plain = 5
    print(plain)
}
