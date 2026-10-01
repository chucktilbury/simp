start {
    float signedMinimum = float(-9223372036854775808)
    float signedLastRepresentable = float(9223372036854774784)
    float unsignedLastRepresentable = float(18446744073709549568u)
    print(int(signedMinimum))
    print(int(signedLastRepresentable))
    print(unsigned(unsignedLastRepresentable))
    print(int(-3.99))
    print(unsigned(3.99))

    try {
        print(int(float(9223372036854775807)))
    } except() as error {
        print(error)
    }

    try {
        print(int(-9223372036854777856.0))
    } except() as error {
        print(error)
    }

    try {
        print(int(0.0 / 0.0))
    } except() as error {
        print(error)
    }

    try {
        print(unsigned(-1.0))
    } except() as error {
        print(error)
    }

    try {
        print(unsigned(float(18446744073709551615u)))
    } except() as error {
        print(error)
    }

    int minimum = -9223372036854775808
    try {
        print(minimum / -1)
    } except() as error {
        print(error)
    }

    try {
        print(minimum % -1)
    } except() as error {
        print(error)
    }
}
