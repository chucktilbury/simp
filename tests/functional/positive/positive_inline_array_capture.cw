# Passes an list into inline C and verifies its length is visible both inside C and in Cwhip.
start {
    list values = [1, 2, 3]
    inline (list values) {
        printf("%llu\n", (unsigned long long)((CwhipArray *)*values)->length);
    }
    print(values.length)
}
