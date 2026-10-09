set(CASE_NAME cwhip_rejects_duplicate_virtual_base_initializer)
set(CASE_FIXTURE negative_virtual_base_duplicate_initializer.cw)
set(CASE_EXPECTED_DIAGNOSTIC [==[virtual base 'Root' is initialized more than once]==])
