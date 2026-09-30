set(CASE_NAME simp_rejects_writing_exception_binding)
set(CASE_FIXTURE negative_exception_binding_assignment.simp)
set(CASE_EXPECTED_DIAGNOSTIC [==[exception binding 'message' is read-only]==])
