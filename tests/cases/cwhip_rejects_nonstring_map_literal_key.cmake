set(CASE_NAME cwhip_rejects_nonstring_map_literal_key)
set(CASE_FIXTURE negative_map_literal_key.cw)
set(CASE_EXPECTED_DIAGNOSTIC [==[dict keys must have type strg]==])
