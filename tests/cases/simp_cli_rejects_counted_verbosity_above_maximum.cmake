set(CASE_NAME simp_cli_rejects_counted_verbosity_above_maximum)
set(CASE_FIXTURE positive_integer_output.simp)
set(CASE_ARGUMENTS -vvvv --check-only)
set(CASE_EXPECTED_DIAGNOSTIC [==[cwhip: verbosity level 4 exceeds the maximum of 3]==])
