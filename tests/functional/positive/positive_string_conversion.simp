# Exercises string-to-number conversion (s.toInt(), s.toUnsigned(),
# s.toFloat()): the built-in, non-class dot-operations on 'string', in the
# same style as list.length/buffer.resize(). Covers success, a leading
# '+', overflow, invalid characters (including trailing garbage and an
# empty string), a negative sign rejected by toUnsigned, and a null
# receiver raising instead of crashing.
start {
    strg a = "42"
    strg b = "-17"
    strg c = "3.14"
    strg d = "255"

    print(a.toInt())
    print(b.toInt())
    print(d.toUnsigned())
    print(c.toFloat())

    strg plus = "+5"
    print(plus.toInt())

    strg sci = "1.5e2"
    print(sci.toFloat())

    strg maxUnsigned = "18446744073709551615"
    print(maxUnsigned.toUnsigned())

    try {
        strg bad = "abc"
        print(bad.toInt())
    } except() as e {
        print(e)
    }

    try {
        strg trailing = "12x"
        print(trailing.toInt())
    } except() as e {
        print(e)
    }

    try {
        strg empty = ""
        print(empty.toInt())
    } except() as e {
        print(e)
    }

    try {
        strg overflow = "9223372036854775808"
        print(overflow.toInt())
    } except() as e {
        print(e)
    }

    try {
        strg negative = "-5"
        print(negative.toUnsigned())
    } except() as e {
        print(e)
    }

    try {
        strg n = null
        print(n.toInt())
    } except() as e {
        print("null reference")
    }
}
