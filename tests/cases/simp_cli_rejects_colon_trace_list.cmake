set(CASE_NAME simp_cli_rejects_colon_trace_list)
set(CASE_FIXTURE positive_integer_output.simp)
set(CASE_ARGUMENTS -t parser:scanner --check-only)
set(CASE_EXPECTED_DIAGNOSTIC [==[simp: unsupported trace target: parser:scanner]==])
