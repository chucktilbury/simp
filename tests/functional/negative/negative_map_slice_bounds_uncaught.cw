# Raises an uncaught runtime error when a dict slice extends beyond its entries.
start {
    dict values = {"first": 1, "second": 2}
    dict copy = values[0:3]
    print(copy.length)
}
