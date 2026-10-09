set(CASE_NAME cwhip_rejects_buffer_slice_step)
set(CASE_FIXTURE negative_buffer_slice_step.cw)
set(CASE_EXPECTED_DIAGNOSTIC [==[slice steps are only supported for lists]==])
