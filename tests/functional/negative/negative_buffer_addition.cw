# Rejects using a byte buffer as an arithmetic operand.
start {
    buffer first = buffer(1)
    buffer second = buffer(1)
    buffer combined = first + second
}
