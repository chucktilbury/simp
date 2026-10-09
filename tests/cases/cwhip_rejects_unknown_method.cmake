set(CASE_NAME cwhip_rejects_unknown_method)
set(CASE_FIXTURE negative_unknown_method.cw)
set(CASE_EXPECTED_DIAGNOSTIC [==[class 'Counter' has no method 'missing']==])
