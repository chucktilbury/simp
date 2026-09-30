set(CASE_NAME simp_rejects_array_slice_non_integer_step)
set(CASE_FIXTURE negative_array_slice_non_integer_step.simp)
set(CASE_EXPECTED_DIAGNOSTIC [==[array slice step must be int]==])
