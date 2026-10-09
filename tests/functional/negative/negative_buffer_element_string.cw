# Rejects writing a string into a byte buffer.
start {
    buffer bytes = buffer(1)
    bytes[0] = "byte"
}
