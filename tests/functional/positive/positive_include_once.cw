# Includes the same source twice and verifies it is processed only once.
include "../include_parts/once.cw"
include "../include_parts/once.cw"

start {
    IncludedOnce value = IncludedOnce()
    print(value.answer())
}
