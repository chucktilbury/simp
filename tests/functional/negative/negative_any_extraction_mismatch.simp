# Rejects extracting a string-valued list element into an int at runtime.
start {
    // Extracting a concrete type from 'any' is runtime-checked: this list
    // slot holds a string, so extracting it into 'int' must raise instead of
    // silently reinterpreting the bits.
    list values = [1, "two", 3]
    int mismatched = values[1]
    print(mismatched)
}
