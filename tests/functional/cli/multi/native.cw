class Native {
    int get()
}
int Native.get() from "cwhip_multi_external_value"
start {
    Native native = Native()
    print(native.get())
}
