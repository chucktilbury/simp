import shared as Shared

class Alpha {
    int version() {
        Shared shared = Shared()
        return shared.version()
    }
}
