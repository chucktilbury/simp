# Rejects reading a private field through an external object reference.
class Hidden {
    private:
    int value
    int read() {
        return value
    }
}

start {
    Hidden hidden = Hidden()
    print(hidden.value)
}
