set(CASE_NAME simp_rejects_incompatible_override)
set(CASE_FIXTURE negative_incompatible_override.simp)
set(CASE_EXPECTED_DIAGNOSTIC [==[override of 'value' must preserve the inherited method signature]==])
