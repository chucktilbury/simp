# Rejects a non-integer list slice step.
start {
    list values = [1, 2, 3]
    list invalid = values[::"two"]
}
