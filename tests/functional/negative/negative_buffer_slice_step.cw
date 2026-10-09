# Rejects steps on buffer slices.
start {
    buffer values = buffer(1)
    buffer invalid = values[::2]
}
