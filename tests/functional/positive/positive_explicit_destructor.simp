# Verifies an explicit destroy call invokes a class destructor and can access the object state.
class Resource {
    int code
    Resource(int value) {
        code = value
    }
    destroy {
        print(code)
    }
}

start {
    Resource resource = Resource(42)
    resource.destroy()
}
