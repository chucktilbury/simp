set(CASE_NAME cwhip_rejects_private_method_access)
set(CASE_FIXTURE negative_private_method.cw)
set(CASE_EXPECTED_DIAGNOSTIC [==[method 'read' is not accessible]==])
