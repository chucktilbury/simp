set(CASE_NAME cwhip_rejects_method_declaration_without_definition)
set(CASE_FIXTURE negative_extern_missing_definition.cw)
set(CASE_EXPECTED_DIAGNOSTIC [==[is declared but has no out-of-line definition]==])
