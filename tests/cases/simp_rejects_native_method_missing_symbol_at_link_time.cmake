set(CASE_NAME simp_rejects_native_method_missing_symbol_at_link_time)
set(CASE_FIXTURE negative_extern_missing_symbol.simp)
set(CASE_EXPECTED_DIAGNOSTIC [==[undefined reference to .simp_symbol_that_does_not_exist.]==])
