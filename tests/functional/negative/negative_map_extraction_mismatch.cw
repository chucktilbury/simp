# Raises a runtime error when an integer-valued any is extracted as a dict.
start {
    list values = [7]
    dict invalid = values[0]
}
