set(CASE_NAME cwhip_rejects_signed_int64_below_min)
set(CASE_FIXTURE negative_signed_int64_below_min.cw)
set(CASE_EXPECTED_DIAGNOSTIC [==[integer literal is outside the signed 64-bit range]==])
