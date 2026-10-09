set(CASE_NAME cwhip_rejects_missing_virtual_base_initializer)
set(CASE_FIXTURE negative_virtual_base_missing_initializer.cw)
set(CASE_EXPECTED_DIAGNOSTIC [==[most-derived constructor must initialize virtual base 'Root']==])
