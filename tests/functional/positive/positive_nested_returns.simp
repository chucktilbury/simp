class Payload {
    int code

    Payload(int value) {
        code = value
    }
}

class ReturnProbe {
    int evaluations

    strg makeValue() {
        evaluations = evaluations + 1
        return "survived"
    }

    int choose(bool first) {
        if (first) {
            { return 10 }
        } else {
            return 20
        }
    }

    int fromLoop() {
        int index = 0
        while (index < 3) {
            if (index == 2) { return index }
            index = index + 1
        }
        return -1
    }

    int fromFor() {
        for (item in [1, 2]) {
            int value = item
            if (value == 2) { return value }
        }
        return -1
    }

    int fromHandler() {
        try {
            try {
                raise(Exception("go"))
            } except(Exception) {
                return 7
            } finally {
                print("inner finally")
            }
        } finally {
            print("outer finally")
        }
    }

    strg rootedValue(ReturnProbe self) {
        try {
            return self.makeValue()
        } finally {
            int count = 0
            while (count < 180) {
                strg temporary = "temporary"
                count = count + 1
            }
            print(evaluations)
        }
    }

    Payload rootedObject() {
        try {
            return Payload(99)
        } finally {
            int count = 0
            while (count < 180) {
                Payload temporary = Payload(count)
                count = count + 1
            }
        }
    }

    int replacedByException() {
        try {
            return 8
        } finally {
            raise(Exception("replacement"))
        }
    }

    void earlyExit() {
        do {
            try {
                return
            } finally {
                print("void finally")
            }
        } while (false)
        print("unreachable")
    }
}

start {
    ReturnProbe probe = ReturnProbe()
    print(probe.choose(true))
    print(probe.choose(false))
    print(probe.fromLoop())
    print(probe.fromFor())
    print(probe.fromHandler())
    print(probe.rootedValue(probe))
    print(probe.rootedObject().code)
    try {
        print(probe.replacedByException())
    } except(Exception) as caught {
        print(caught.message)
    }
    probe.earlyExit()
}
