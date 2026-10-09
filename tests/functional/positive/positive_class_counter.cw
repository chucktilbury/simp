# Verifies object fields can be updated by methods and then read through both methods and direct member access.
class Counter {
    int value

    Counter(int initial) {
        value = initial
    }

    int add(int amount) {
        value = value + amount
        return value
    }

    int get() {
        return value
    }
}

start {
    Counter counter = null
    counter = Counter(40)
    print(counter.add(2))
    print(counter.get())
    print(counter.value)
}
