set(CASE_NAME simp_rejects_bool_arithmetic)
set(CASE_FIXTURE negative_bool_arithmetic.simp)
set(CASE_EXPECTED_DIAGNOSTIC [==[requires matching int, float, or unsigned operands]==])
