set(CASE_NAME simp_rejects_void_typed_constructor)
set(CASE_FIXTURE negative_void_typed_constructor.simp)
set(CASE_EXPECTED_DIAGNOSTIC [==[negative_void_typed_constructor\.simp:2:8: error: constructor 'SomeClass' must omit the return type; write 'SomeClass\(\.\.\.\)' instead of 'void SomeClass\(\.\.\.\)']==])
