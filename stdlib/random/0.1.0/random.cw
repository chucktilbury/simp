namespace Random {
    class SecureRandom {
        buffer bytes(int size)
        bool fill(buffer target)
    }

    buffer SecureRandom.bytes(int size) from "cwhip_random_bytes"
    bool SecureRandom.fill(buffer target) from "cwhip_random_fill"
}
