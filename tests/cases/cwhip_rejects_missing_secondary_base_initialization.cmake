set(CASE_NAME cwhip_rejects_missing_secondary_base_initialization)
set(CASE_FIXTURE negative_secondary_base_constructor.cw)
set(CASE_EXPECTED_DIAGNOSTIC [==[derived constructor must initialize base 'Secondary']==])
