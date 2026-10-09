set(CASE_NAME cwhip_rejects_non_integer_array_index)
set(CASE_FIXTURE negative_array_index_type.cw)
set(CASE_EXPECTED_DIAGNOSTIC [==[list index and slice bounds must be int]==])
