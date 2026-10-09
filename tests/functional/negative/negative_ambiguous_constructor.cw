class First {}
class Second {}

class Ambiguous {
    Ambiguous(First value) {}
    Ambiguous(Second value) {}
}

start {
    Ambiguous value = Ambiguous(null)
}
