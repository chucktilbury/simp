# Rejects non-boolean operands to keyword and symbolic logical operators.
start {
    strg value = "not an integer"
    bool invalid = value and true
}
