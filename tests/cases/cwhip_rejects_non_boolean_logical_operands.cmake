set(CASE_NAME cwhip_rejects_non_boolean_logical_operands)
set(CASE_FIXTURE negative_logical_operand_type.cw)
set(CASE_EXPECTED_DIAGNOSTIC [==[logical operator '&&' requires bool operands]==])
