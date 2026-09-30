set(CASE_NAME simp_rejects_missing_secondary_base_initialization)
set(CASE_FIXTURE negative_secondary_base_constructor.simp)
set(CASE_EXPECTED_DIAGNOSTIC [==[derived constructor must initialize base 'Secondary']==])
