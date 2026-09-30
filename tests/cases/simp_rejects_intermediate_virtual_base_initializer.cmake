set(CASE_NAME simp_rejects_intermediate_virtual_base_initializer)
set(CASE_FIXTURE negative_virtual_base_initializer_in_intermediate.simp)
set(CASE_EXPECTED_DIAGNOSTIC [==[virtual base initializers are only allowed in most-derived classes]==])
