set(CASE_NAME cwhip_rejects_private_inherited_member_access)
set(CASE_FIXTURE negative_private_inheritance.cw)
set(CASE_EXPECTED_DIAGNOSTIC [==[is not accessible through this inheritance path]==])
