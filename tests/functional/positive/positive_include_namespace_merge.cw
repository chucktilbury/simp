# Checks declarations from separate includes merge into the same namespace.
include "../include_parts/namespace_left.cw"
include "../include_parts/namespace_right.cw"

start {
    Library.Left left = Library.Left()
    Library.Right right = Library.Right()
    print(left.answer())
    print(right.answer())
}
