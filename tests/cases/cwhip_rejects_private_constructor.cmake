set(CASE_NAME cwhip_rejects_private_constructor)
set(CASE_FIXTURE negative_private_constructor.cw)
set(CASE_EXPECTED_DIAGNOSTIC [==[constructor for class 'Hidden' is not accessible]==])
