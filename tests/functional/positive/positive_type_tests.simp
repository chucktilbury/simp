# Exercises static and dynamic type tests, subclass matching, and bool conditions.
class Root {}
class Middle : Root {}
class Leaf : Middle {}
class Other {}

namespace shapes {
    class Named {}
}

start {
    int integer = 7
    print(integer is int)
    print(integer is float)
    print(1 + 2 * 3 is int == true)

    Leaf leaf = Leaf()
    Root baseReference = leaf
    Root rootOnly = Root()
    Other unrelated = Other()
    print(leaf is Leaf)
    print(leaf is Root)
    print(baseReference is Leaf)
    print(rootOnly is Leaf)
    print(unrelated is Root)
    print(leaf is Middle)

    shapes.Named named = shapes.Named()
    print(named is shapes.Named)

    dict values = {"integer": 42, "text": "hello", "real": 3.5, "dictionary": {"key": 1}, "sequence": [1, 2], "object": leaf, "nothing": null, "flag": true, "count": 5u}
    print(values["integer"] is int)
    print(values["integer"] is strg)
    print(values["text"] is strg)
    print(values["text"] is float)
    print(values["real"] is float)
    print(values["dictionary"] is dict)
    print(values["dictionary"] is dict)
    print(values["dictionary"] is list)
    print(values["sequence"] is list)
    print(values["sequence"] is list)
    print(values["sequence"] is dict)
    print(values["object"] is Leaf)
    print(values["object"] is Root)
    print(values["object"] is Other)
    print(values["flag"] is bool)
    print(values["count"] is unsigned)
    print(values["nothing"] is int)
    print(values["nothing"] is strg)
    print(values["nothing"] is Root)

    Root emptyObject = null
    strg emptyText = null
    list emptyArray = null
    print(emptyObject is Root)
    print(emptyText is strg)
    print(emptyArray is list)
    int emptyInteger = null
    print(emptyInteger is int)
    handle emptyHandle = null
    print(emptyHandle is handle)
    buffer storage = buffer(1)
    print(storage is buffer)

    if (leaf is Root) {
        print("if type test")
    }
    bool condition = leaf is Leaf
    print(condition)
    int iterations = 0
    Root current = Root()
    while (current is Root and iterations < 1) {
        iterations = iterations + 1
        current = null
    }
    print(iterations)
}
