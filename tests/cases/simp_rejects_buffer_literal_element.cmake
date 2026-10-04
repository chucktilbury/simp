set(CASE_NAME simp_rejects_buffer_literal_element)
set(CASE_FIXTURE negative_buffer_literal_element.simp)
set(CASE_EXPECTED_DIAGNOSTIC [==[buffer literal elements must have type int or unsigned]==])
