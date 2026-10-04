# All sixteen ordered scalar pairs, including identities.
class Scalars {
    bool flag() { return true }
    int signedValue() { return -7 }
    unsigned unsignedValue() { return 12u }
    float fraction() { return 3.75 }
    int fromBool(bool value) { return int(value) }
}

start {
    int i = 7
    unsigned u = 12
    float f = 3.75

    print(float(i))
    print(float(u))
    print(int(f))
    print(unsigned(f))
    print(int(-3.99))

    float sum = float(i) + f
    print(sum)

    bool b = true
    print(bool(b))
    print(int(b))
    print(unsigned(b))
    print(float(b))
    print(bool(i))
    print(int(i))
    print(unsigned(i))
    print(bool(u))
    print(int(u))
    print(unsigned(u))
    print(bool(f))
    print(float(f))

    print(int(false))
    print(unsigned(false))
    print(float(false))
    print(bool(0))
    print(bool(0u))
    print(bool(-1))
    print(bool(-9223372036854775808))
    print(bool(18446744073709551615u))
    print(int(true))
    print(unsigned(true))
    print(float(true))

    Scalars values = Scalars()
    print(int(values.flag()))
    print(bool(values.signedValue()))
    print(int(values.unsignedValue()))
    print(unsigned(values.fraction()))
    i = int(values.fraction())
    u = unsigned(i)
    f = float(u)
    b = bool(f)
    print(i)
    print(u)
    print(f)
    print(b)
    print(values.fromBool(bool(2)))

    # Casts consume null flags, including identity casts.
    bool nullBool = null
    int nullInt = null
    unsigned nullUnsigned = null
    float nullFloat = null
    print(bool(nullBool))
    print(int(nullBool))
    print(unsigned(nullBool))
    print(float(nullBool))
    print(bool(nullInt))
    print(int(nullInt))
    print(unsigned(nullInt))
    print(float(nullInt))
    print(bool(nullUnsigned))
    print(int(nullUnsigned))
    print(unsigned(nullUnsigned))
    print(float(nullUnsigned))
    print(bool(nullFloat))
    print(int(nullFloat))
    print(unsigned(nullFloat))
    print(float(nullFloat))
    nullBool = bool(nullBool)
    nullInt = int(nullInt)
    nullUnsigned = unsigned(nullUnsigned)
    nullFloat = float(nullFloat)
    print(nullBool == null)
    print(nullInt == null)
    print(nullUnsigned == null)
    print(nullFloat == null)
    print(type(bool))
}
