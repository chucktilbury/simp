# Passes a Cwhip string into inline C and prints it through the runtime string helper.
start {
    strg message = "hello from C"
    inline (strg message) {
        printf("%s\n", cwhip_string_cstr(message));
    }
}
