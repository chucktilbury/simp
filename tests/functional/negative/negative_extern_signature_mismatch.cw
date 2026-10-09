# Rejects an out-of-line definition whose parameter and return types differ from its declaration.
class Native {
    int absolute(int value)
}

strg Native.absolute(strg value) from "cwhip_method_demo_abs"

start {
    print(1)
}
