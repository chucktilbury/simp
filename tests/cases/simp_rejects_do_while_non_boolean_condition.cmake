set(CASE_NAME simp_rejects_do_while_non_boolean_condition)
set(CASE_FIXTURE negative_do_while_condition.simp)
set(CASE_EXPECTED_DIAGNOSTIC [==[while condition must have type bool]==])
