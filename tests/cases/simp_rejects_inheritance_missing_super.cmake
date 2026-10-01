set(CASE_NAME simp_rejects_inheritance_missing_super)
set(CASE_FIXTURE negative_inheritance_missing_super.simp)
set(CASE_EXPECTED_DIAGNOSTIC [==[derived constructor must begin with super Base]==])
