set(CASE_NAME cwhip_cli_rejects_colon_trace_list)
set(CASE_FIXTURE positive_integer_output.cw)
set(CASE_ARGUMENTS -t parser:scanner --check-only)
set(CASE_EXPECTED_DIAGNOSTIC [==[cwhip: unsupported trace target: parser:scanner]==])
