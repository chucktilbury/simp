# Rejects equality between two byte buffers while null comparison remains allowed.
start {
    buffer first = buffer(1)
    buffer second = buffer(1)
    bool same = first == second
}
