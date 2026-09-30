set(CASE_NAME simp_rejects_protected_inherited_member_access)
set(CASE_FIXTURE negative_protected_inheritance.simp)
set(CASE_EXPECTED_DIAGNOSTIC [==[is not accessible through this inheritance path]==])
