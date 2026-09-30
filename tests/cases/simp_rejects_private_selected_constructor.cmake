set(CASE_NAME simp_rejects_private_selected_constructor)
set(CASE_FIXTURE negative_private_selected_constructor.simp)
set(CASE_EXPECTED_DIAGNOSTIC [==[constructor for class 'ProtectedChoice' is not accessible]==])
