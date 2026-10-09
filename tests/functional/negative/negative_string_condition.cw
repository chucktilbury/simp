# Rejects using a string expression as an if condition, which requires bool.
start {
    if ("not an integer") {
        print(1)
    }
}
