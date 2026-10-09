set(CASE_NAME cwhip_cli_rejects_removed_trace_parser)
set(CASE_FIXTURE positive_integer_output.cw)
set(CASE_ARGUMENTS --trace-parser --check-only)
set(CASE_EXPECTED_DIAGNOSTIC [==[cwhip: unknown option: --trace-parser]==])
