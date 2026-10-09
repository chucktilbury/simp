# Verifies cyclic source includes terminate and expose declarations from the included cycle.
include "../include_parts/cycle_a.cw"

start {
    CycleValue value = CycleValue()
    print(value.answer())
}
