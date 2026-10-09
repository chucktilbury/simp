class IntBox {
    int value

    IntBox(int initial) {
        value = initial
    }

    int get() {
        return value
    }

    int echo(int argument) {
        return argument
    }
}

class Native {
    int marker

    Native(int initialMarker) {
        marker = initialMarker
    }

    int absolute(int value)
}

int Native.absolute(int value) from "cwhip_method_demo_abs"

start {
    int above32 = 2147483648
    int minimum = -9223372036854775808
    int maximum = 9223372036854775807

    print(above32)
    print(minimum)
    print(maximum)
    print(above32 + 1)
    print(above32 * 2)
    print(minimum < -2147483648)
    print(minimum < maximum)
    print(maximum > above32)
    print(minimum + 1)

    IntBox box = IntBox(maximum)
    print(box.get())
    print(box.echo(minimum))

    Native native = Native(91)
    print(native.absolute(above32))
    print(native.absolute(maximum))

    list values = [minimum, maximum, above32]
    print(values[0])
    print(values[1])
    int arrayValue = values[2]
    print(arrayValue)
    try {
        int invalidIndex = values[4294967296]
        print("unexpected list index")
    } except(Exception) as error {
        print(error.message)
    }

    dict bounds = {"minimum": minimum, "maximum": maximum}
    print(bounds["minimum"])
    print(bounds["maximum"])

    strg minText = "-9223372036854775808"
    strg maxText = "9223372036854775807"
    print(minText.toInt())
    print(maxText.toInt())

    try {
        strg aboveMaxText = "9223372036854775808"
        print(aboveMaxText.toInt())
    } except() as error {
        print(error)
    }

    try {
        strg belowMinText = "-9223372036854775809"
        print(belowMinText.toInt())
    } except() as error {
        print(error)
    }
}
