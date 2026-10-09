start {
    strg value = "valid"
    try {
        inline (strg value) {
            const char invalid[] = { (char)0xc0 };
            cwhip_string_append_bytes(*value, invalid, 1);
        }
    } except() as error {
        print(error)
    }
    print(value)
}
