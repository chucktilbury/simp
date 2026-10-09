start {
    buffer bytes = buffer(2)
    inline (buffer bytes) {
        cwhip_buffer_resize(*bytes, 3, "<inline>", 8, 1, 1);
        cwhip_buffer_set(*bytes, 0, 65, "<inline>", 8, 1, 1);
    }
    print(bytes.length)
    print(bytes[0])
}
