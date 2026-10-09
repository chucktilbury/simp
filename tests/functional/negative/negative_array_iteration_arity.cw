# Rejects binding both a key and value while iterating an list, which supplies values only.
start {
    list values = [1, 2]
    for (key, value in values) {
        print(value)
    }
}
