set(CASE_NAME cwhip_rejects_protected_inherited_member_access)
set(CASE_FIXTURE negative_protected_inheritance.cw)
set(CASE_EXPECTED_DIAGNOSTIC [==[is not accessible through this inheritance path]==])
