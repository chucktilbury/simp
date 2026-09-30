set(CASE_NAME simp_rejects_uninitialized_variable)
set(CASE_FIXTURE negative_uninitialized.simp)
set(CASE_EXPECTED_DIAGNOSTIC [==[variable 'value' may be uninitialized]==])
