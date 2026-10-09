set(CASE_NAME cwhip_cli_rejects_mixed_verbosity_forms)
set(CASE_FIXTURE positive_integer_output.cw)
set(CASE_ARGUMENTS -v --verbosity=1 --check-only)
set(CASE_EXPECTED_DIAGNOSTIC [==[cwhip: -v and --verbosity cannot be used together]==])
