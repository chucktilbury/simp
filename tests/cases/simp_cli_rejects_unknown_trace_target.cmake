set(CASE_NAME simp_cli_rejects_unknown_trace_target)
set(CASE_FIXTURE positive_integer_output.simp)
set(CASE_ARGUMENTS -t parser,tokens --check-only)
set(CASE_EXPECTED_DIAGNOSTIC [==[simp: unsupported trace target: tokens \(expected scanner, parser, ast, or symbols\)]==])
