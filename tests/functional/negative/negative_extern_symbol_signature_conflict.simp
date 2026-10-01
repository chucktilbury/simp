# Rejects reusing one C symbol for native methods with incompatible ABI signatures.
class Native {
    int integerValue(int value)
    strg stringValue(strg value)
}

int Native.integerValue(int value) from "same_c_symbol"
strg Native.stringValue(strg value) from "same_c_symbol"

start {
    print(1)
}
