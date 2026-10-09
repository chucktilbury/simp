set(CASE_NAME simp_cli_rejects_removed_trace_parser)
set(CASE_FIXTURE positive_integer_output.simp)
set(CASE_ARGUMENTS --trace-parser --check-only)
set(CASE_EXPECTED_DIAGNOSTIC [==[cwhip: unknown option: --trace-parser]==])
