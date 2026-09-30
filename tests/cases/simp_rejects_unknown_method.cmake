set(CASE_NAME simp_rejects_unknown_method)
set(CASE_FIXTURE negative_unknown_method.simp)
set(CASE_EXPECTED_DIAGNOSTIC [==[class 'Counter' has no method 'missing']==])
