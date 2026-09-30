set(CASE_NAME simp_rejects_unsigned_int_mixing)
set(CASE_FIXTURE negative_unsigned_int_mixing.simp)
set(CASE_EXPECTED_DIAGNOSTIC [==[requires matching int, float, or unsigned operands]==])
