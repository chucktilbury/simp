include "nested_value.cw"
class SearchTarget {
    int answer() {
        NestedValue nested = NestedValue()
        return 40 + nested.value()
    }
}
