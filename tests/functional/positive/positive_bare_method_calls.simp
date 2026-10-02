class Counter {
    int value

    void step() {
        value = value + 1
    }

    void run() {
        step()
        print(value)
    }

    int sum(int n) {
        if (n == 0) {
            return 0
        }
        return n + sum(n - 1)
    }
}

start {
    Counter counter = Counter()
    counter.run()
    print(counter.sum(4))
}
