set(CASE_NAME cwhip_cli_rejects_counted_verbosity_above_maximum)
set(CASE_FIXTURE positive_integer_output.cw)
set(CASE_ARGUMENTS -vvvv --check-only)
set(CASE_EXPECTED_DIAGNOSTIC [==[cwhip: verbosity level 4 exceeds the maximum of 3]==])
