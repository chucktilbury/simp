set(CASE_NAME simp_rejects_non_integer_array_index)
set(CASE_FIXTURE negative_array_index_type.simp)
set(CASE_EXPECTED_DIAGNOSTIC [==[array index and slice bounds must be int]==])
