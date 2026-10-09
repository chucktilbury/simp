# Rejects accessing a field absent from the Counter class.
class Counter {
    int value
}

start {
    Counter counter = Counter()
    print(counter.missing)
}
