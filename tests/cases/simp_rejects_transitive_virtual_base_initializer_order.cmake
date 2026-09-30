set(CASE_NAME simp_rejects_transitive_virtual_base_initializer_order)
set(CASE_FIXTURE negative_transitive_virtual_base_initializer_order.simp)
set(CASE_EXPECTED_DIAGNOSTIC [==[virtual base initializers must follow virtual-base construction order]==])
