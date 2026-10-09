
start {

    ; Deliberately read beyond an empty buffer to demonstrate a runtime diagnostic.
    buffer c = buffer(0)
    int ch
    ch = int(c[0]) ; << RT error: invalid buffer reference
    print(ch)
}