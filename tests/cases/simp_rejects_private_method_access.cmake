set(CASE_NAME simp_rejects_private_method_access)
set(CASE_FIXTURE negative_private_method.simp)
set(CASE_EXPECTED_DIAGNOSTIC [==[method 'read' is not accessible]==])
