# Rejects a void-typed inline C capture because it cannot represent a variable value.
start {
    inline (void value) {
        (void)value;
    }
}
