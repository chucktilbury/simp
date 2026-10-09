set(CASE_NAME simp_cli_rejects_removed_verbose)
set(CASE_FIXTURE positive_integer_output.simp)
set(CASE_ARGUMENTS --verbose --check-only)
set(CASE_EXPECTED_DIAGNOSTIC [==[cwhip: unknown option: --verbose]==])
