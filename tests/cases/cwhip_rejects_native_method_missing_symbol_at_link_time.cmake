set(CASE_NAME cwhip_rejects_native_method_missing_symbol_at_link_time)
set(CASE_FIXTURE negative_extern_missing_symbol.cw)
set(CASE_EXPECTED_DIAGNOSTIC [==[undefined reference to .cwhip_symbol_that_does_not_exist.]==])
