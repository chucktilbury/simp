set(CASE_NAME simp_rejects_non_boolean_logical_operands)
set(CASE_FIXTURE negative_logical_operand_type.simp)
set(CASE_EXPECTED_DIAGNOSTIC [==[logical operator '&&' requires bool operands]==])
