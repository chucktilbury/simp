set(CASE_NAME simp_rejects_nonstring_map_literal_key)
set(CASE_FIXTURE negative_map_literal_key.simp)
set(CASE_EXPECTED_DIAGNOSTIC [==[map keys must have type string]==])
