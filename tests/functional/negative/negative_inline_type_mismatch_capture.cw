# Rejects declaring a string variable as an int capture in inline C.
start {
    strg value = "text"
    inline (int value) {
        (void)value;
    }
}
