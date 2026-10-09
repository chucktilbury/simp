# Rejects construction through a qualified class name absent from the declared namespace.
namespace Known {
    class Present {}
}

start {
    Known.Missing item = Known.Missing()
}
