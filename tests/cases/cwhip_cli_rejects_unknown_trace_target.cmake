set(CASE_NAME cwhip_cli_rejects_unknown_trace_target)
set(CASE_FIXTURE positive_integer_output.cw)
set(CASE_ARGUMENTS -t parser,tokens --check-only)
set(CASE_EXPECTED_DIAGNOSTIC [==[cwhip: unsupported trace target: tokens \(expected scanner, parser, ast, or symbols\)]==])
