set(CASE_NAME simp_cli_rejects_removed_dump_ast)
set(CASE_FIXTURE positive_integer_output.simp)
set(CASE_ARGUMENTS --dump-ast --check-only)
set(CASE_EXPECTED_DIAGNOSTIC [==[cwhip: unknown option: --dump-ast]==])
