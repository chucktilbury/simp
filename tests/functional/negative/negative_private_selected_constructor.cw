class ProtectedChoice {
    ProtectedChoice(int value) {}

    private:
    ProtectedChoice(strg value) {}
}

start {
    ProtectedChoice value = ProtectedChoice("private")
}
