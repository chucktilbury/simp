# Rejects a native-bound method call with more arguments than its declaration.
class Native {
    int absolute(int value)
}

int Native.absolute(int value) from "cwhip_method_demo_abs"

start {
    print(Native().absolute(1, 2))
}
