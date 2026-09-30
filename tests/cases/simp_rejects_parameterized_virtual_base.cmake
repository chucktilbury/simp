set(CASE_NAME simp_rejects_parameterized_virtual_base)
set(CASE_FIXTURE negative_virtual_base_constructor_args.simp)
set(CASE_EXPECTED_DIAGNOSTIC [==[most-derived constructor must initialize virtual base 'Root']==])
