# Exercises the permitted uses of collection values without declaring any.
class Holder {
    int value
    void set(int input) { value = input }
    int get() { return value }
}

class Reader {
    int read(list values) { return values[0] }
}

start {
    list values = [42, "text", null]
    int extracted = values[0]
    Holder holder = Holder()
    holder.value = values[0]
    holder.set(values[0])
    int returned = Reader().read(values)

    list nested = [values[0]]
    nested.append(values[0])
    nested[0] = values[0]
    dict mapped = {"value": values[0]}
    mapped["value"] = values[0]

    print(values[0])
    print(format("formatted {}", values[0]))
    print(values[0] is int)
    type dynamicType = type(values[0])
    print(dynamicType)
    bool isNull = values[2] == null
    print(isNull)
    print(values[0] != null)

    for (item in values) {
        print(item)
    }
}
