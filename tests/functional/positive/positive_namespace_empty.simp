# Checks empty namespaces are accepted alongside nested namespace-qualified class construction.
namespace Empty { }

namespace Outer {
    namespace Inner {
        class Item {}
    }
}

start {
    Outer.Inner.Item item = Outer.Inner.Item()
    print(42)
}
