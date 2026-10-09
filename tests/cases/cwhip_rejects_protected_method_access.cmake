set(CASE_NAME cwhip_rejects_protected_method_access)
set(CASE_FIXTURE negative_protected_member.cw)
set(CASE_EXPECTED_DIAGNOSTIC [==[method 'read' is not accessible]==])
