# Checks an included entry file can itself resolve a nested include relative to its location.
include "../include_parts/relative/entry.cw"

start {
    RelativeValue value = RelativeValue()
    print(value.answer())
}
