set(CASE_NAME cwhip_rejects_int_typed_constructor)
set(CASE_FIXTURE negative_int_typed_constructor.cw)
set(CASE_EXPECTED_DIAGNOSTIC [==[negative_int_typed_constructor\.cw:2:7: error: constructor 'Counter' must omit the return type; write 'Counter\(\.\.\.\)' instead of 'int Counter\(\.\.\.\)']==])
