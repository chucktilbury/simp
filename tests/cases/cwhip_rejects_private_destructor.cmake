set(CASE_NAME cwhip_rejects_private_destructor)
set(CASE_FIXTURE negative_private_destructor.cw)
set(CASE_EXPECTED_DIAGNOSTIC [==[destructor for class 'Hidden' is not accessible here]==])
