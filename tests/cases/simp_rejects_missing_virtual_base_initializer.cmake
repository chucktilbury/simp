set(CASE_NAME simp_rejects_missing_virtual_base_initializer)
set(CASE_FIXTURE negative_virtual_base_missing_initializer.simp)
set(CASE_EXPECTED_DIAGNOSTIC [==[most-derived constructor must initialize virtual base 'Root']==])
