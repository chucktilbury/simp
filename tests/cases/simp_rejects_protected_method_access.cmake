set(CASE_NAME simp_rejects_protected_method_access)
set(CASE_FIXTURE negative_protected_member.simp)
set(CASE_EXPECTED_DIAGNOSTIC [==[method 'read' is not accessible]==])
