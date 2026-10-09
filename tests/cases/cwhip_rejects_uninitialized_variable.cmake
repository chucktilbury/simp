set(CASE_NAME cwhip_rejects_uninitialized_variable)
set(CASE_FIXTURE negative_uninitialized.cw)
set(CASE_EXPECTED_DIAGNOSTIC [==[variable 'value' may be uninitialized]==])
