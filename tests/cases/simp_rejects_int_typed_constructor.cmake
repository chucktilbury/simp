set(CASE_NAME simp_rejects_int_typed_constructor)
set(CASE_FIXTURE negative_int_typed_constructor.simp)
set(CASE_EXPECTED_DIAGNOSTIC [==[negative_int_typed_constructor\.simp:2:7: error: constructor 'Counter' must omit the return type; write 'Counter\(\.\.\.\)' instead of 'int Counter\(\.\.\.\)']==])
