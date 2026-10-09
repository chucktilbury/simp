# Signed minimum divided by negative one must use the ordinary overflow check.
start {
    int value = -9223372036854775808
    value /= -1
}
