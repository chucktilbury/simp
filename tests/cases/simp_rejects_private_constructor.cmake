set(CASE_NAME simp_rejects_private_constructor)
set(CASE_FIXTURE negative_private_constructor.simp)
set(CASE_EXPECTED_DIAGNOSTIC [==[constructor for class 'Hidden' is not accessible]==])
