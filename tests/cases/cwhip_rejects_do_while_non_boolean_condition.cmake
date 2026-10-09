set(CASE_NAME cwhip_rejects_do_while_non_boolean_condition)
set(CASE_FIXTURE negative_do_while_condition.cw)
set(CASE_EXPECTED_DIAGNOSTIC [==[while condition must have type bool]==])
