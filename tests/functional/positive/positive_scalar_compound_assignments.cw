# Exercises scalar compound arithmetic, field targets, and one-time receiver evaluation.
class Box {
    int value
    Box() {}
}

class Factory {
    Box box
    int calls
    Factory(Box value) { box = value }
    Box get() {
        calls += 1
        return box
    }
}

start {
    int signed = 10
    signed += 5
    signed -= 2
    signed *= 3
    signed /= 2
    signed %= 4
    print(signed)

    unsigned count = 20u
    count += 5u
    count -= 5u
    count *= 2u
    count /= 4u
    count %= 3u
    print(count)

    float measure = 5.0
    measure += 1.5
    measure -= 0.5
    measure *= 2.0
    measure /= 3.0
    print(measure)

    Box box = Box()
    box.value = 2
    Factory factory = Factory(box)
    factory.get().value += 3
    print(box.value)
    print(factory.calls)

    int wrapped = 9223372036854775807
    wrapped += 1
    print(wrapped)
}
