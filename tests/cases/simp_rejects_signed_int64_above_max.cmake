set(CASE_NAME simp_rejects_signed_int64_above_max)
set(CASE_FIXTURE negative_signed_int64_above_max.simp)
set(CASE_EXPECTED_DIAGNOSTIC [==[integer literal is outside the signed 64-bit range]==])
