set(CASE_NAME simp_cli_rejects_mixed_verbosity_forms)
set(CASE_FIXTURE positive_integer_output.simp)
set(CASE_ARGUMENTS -v --verbosity=1 --check-only)
set(CASE_EXPECTED_DIAGNOSTIC [==[simp: -v and --verbosity cannot be used together]==])
