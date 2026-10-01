# Rejects a multiple-inheritance constructor that omits initialization of its secondary direct base.
class Primary {
    Primary() {
    }
}

class Secondary {
    Secondary() {
    }
}

class Combined : Primary, Secondary {
    Combined() {
        super Primary()
    }
}

start {
}
