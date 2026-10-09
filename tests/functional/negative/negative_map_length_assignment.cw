# Rejects assigning to the read-only length property of a dict.
start {
    dict values = {}
    values.length = 1
}
