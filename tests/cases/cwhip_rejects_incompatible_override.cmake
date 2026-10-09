set(CASE_NAME cwhip_rejects_incompatible_override)
set(CASE_FIXTURE negative_incompatible_override.cw)
set(CASE_EXPECTED_DIAGNOSTIC [==[override of 'value' must preserve the inherited method signature]==])
