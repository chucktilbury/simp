# Rejects declared any types in native method parameters.
class Native {
    int use(any value)
}

int Native.use(any value) from "some_symbol"

start {
    print(1)
}
