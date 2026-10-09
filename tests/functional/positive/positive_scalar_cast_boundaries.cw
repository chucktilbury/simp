class Conversions {
    unsigned toUnsigned(int value) { return unsigned(value) }
    int toInt(unsigned value) { return int(value) }
    int floatToInt(float value) { return int(value) }
    unsigned floatToUnsigned(float value) { return unsigned(value) }
}

start {
    int x = -10
    unsigned y = unsigned(x)
    print(y)
    print(unsigned(-10))
    Conversions conversions = Conversions()
    print(conversions.toUnsigned(-10))
    print(conversions.toUnsigned(0))
    print(conversions.toUnsigned(9223372036854775806))
    print(conversions.toUnsigned(9223372036854775807))
    print(conversions.toInt(0u))
    print(conversions.toInt(9223372036854775806u))
    print(conversions.toInt(9223372036854775807u))
    print(int(-9223372036854775808))
    print(unsigned(18446744073709551615u))
    print(float(9223372036854775807) == 9223372036854775808.0)
    print(float(18446744073709551615u) == 18446744073709551616.0)

    print(conversions.toUnsigned(-1))
    print(conversions.toUnsigned(-9223372036854775808))
    try { print(conversions.toInt(9223372036854775808u)) }
    except() as error { print(error) }
    try { print(conversions.toInt(18446744073709551615u)) }
    except() as error { print(error) }

    float nan = 0.0 / 0.0
    float infinity = 1.0 / 0.0
    float negativeInfinity = -1.0 / 0.0
    float positiveZero = 0.0
    float negativeZero = -0.0
    float subnormal = 1e-300 / 1e20
    float negativeSubnormal = -subnormal
    print(bool(nan))
    print(bool(infinity))
    print(bool(negativeInfinity))
    print(bool(positiveZero))
    print(bool(negativeZero))
    print(bool(subnormal))
    print(bool(negativeSubnormal))
    print(bool(0.25))
    print(bool(-0.25))
    print(float(nan) != nan)
    print(float(infinity) == infinity)
    print(float(negativeInfinity) == negativeInfinity)
    print(1.0 / float(negativeZero) == negativeInfinity)
    print(float(subnormal) == subnormal)
    print(conversions.floatToInt(positiveZero))
    print(conversions.floatToInt(negativeZero))
    print(conversions.floatToUnsigned(positiveZero))
    print(conversions.floatToUnsigned(negativeZero))
    print(conversions.floatToInt(subnormal))
    print(conversions.floatToInt(negativeSubnormal))
    print(conversions.floatToUnsigned(subnormal))
    print(conversions.floatToInt(-0.25))
    print(conversions.floatToInt(-3.75))
    print(conversions.floatToUnsigned(3.75))
    print(conversions.floatToInt(-9223372036854775808.0))
    print(conversions.floatToInt(9223372036854774784.0))
    print(conversions.floatToUnsigned(18446744073709549568.0))

    try { print(conversions.floatToInt(nan)) }
    except() as error { print(error) }
    try { print(conversions.floatToUnsigned(nan)) }
    except() as error { print(error) }
    try { print(conversions.floatToInt(infinity)) }
    except() as error { print(error) }
    try { print(conversions.floatToUnsigned(infinity)) }
    except() as error { print(error) }
    try { print(conversions.floatToInt(negativeInfinity)) }
    except() as error { print(error) }
    try { print(conversions.floatToUnsigned(negativeInfinity)) }
    except() as error { print(error) }
    try { print(conversions.floatToUnsigned(negativeSubnormal)) }
    except() as error { print(error) }
    try { print(conversions.floatToUnsigned(-0.25)) }
    except() as error { print(error) }
    try { print(conversions.floatToInt(9223372036854775808.0)) }
    except() as error { print(error) }
    try { print(conversions.floatToInt(-9223372036854777856.0)) }
    except() as error { print(error) }
    try { print(conversions.floatToUnsigned(18446744073709551616.0)) }
    except() as error { print(error) }
}
