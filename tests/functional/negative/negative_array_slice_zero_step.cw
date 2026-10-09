# Rejects a slice whose integer-constant step evaluates to zero.
start {
    list values = [1, 2, 3]
    list invalid = values[::1 - 1]
}
