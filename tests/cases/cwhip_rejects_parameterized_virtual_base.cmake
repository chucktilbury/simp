set(CASE_NAME cwhip_rejects_parameterized_virtual_base)
set(CASE_FIXTURE negative_virtual_base_constructor_args.cw)
set(CASE_EXPECTED_DIAGNOSTIC [==[most-derived constructor must initialize virtual base 'Root']==])
