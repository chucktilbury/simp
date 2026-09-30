set(CASE_NAME simp_rejects_buffer_slice_step)
set(CASE_FIXTURE negative_buffer_slice_step.simp)
set(CASE_EXPECTED_DIAGNOSTIC [==[slice steps are only supported for arrays]==])
