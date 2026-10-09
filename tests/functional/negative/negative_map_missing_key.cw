# Raises an uncaught runtime error when indexing a dict with an absent key.
start {
    dict values = {}
    print(values["missing"])
}
