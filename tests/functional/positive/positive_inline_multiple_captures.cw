# Passes both a 64-bit integer and a string into one inline C block.
start {
    int count = 9223372036854775807
    strg message = "answer"
    inline (int count, strg message) {
        printf("%s: %lld\n", cwhip_string_cstr(message), (long long)*count);
    }
}
