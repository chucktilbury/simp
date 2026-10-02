# Raises a runtime error when the same object is explicitly destroyed twice.
class Resource {
    destroy {
    }
}

start {
    Resource resource = Resource()
    resource.destroy()
    resource.destroy()
}
