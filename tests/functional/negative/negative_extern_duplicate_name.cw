# Rejects providing two out-of-line native definitions for the same declared method.
class Native {
    int absolute(int value)
}

int Native.absolute(int value) from "cwhip_method_demo_abs"
int Native.absolute(int value) from "cwhip_method_demo_abs"

start {
    print(1)
}
