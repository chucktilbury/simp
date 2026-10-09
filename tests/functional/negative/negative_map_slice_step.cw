# Rejects steps on insertion-order dict slices.
start {
    dict values = {"a": 1}
    dict invalid = values[::2]
}
