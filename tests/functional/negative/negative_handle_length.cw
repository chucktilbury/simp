# Rejects built-in member access on an opaque handle.
start {
    handle resource = null
    int size = resource.length
}
