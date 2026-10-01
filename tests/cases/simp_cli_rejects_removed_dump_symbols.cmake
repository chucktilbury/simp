set(CASE_NAME simp_cli_rejects_removed_dump_symbols)
set(CASE_FIXTURE positive_integer_output.simp)
set(CASE_ARGUMENTS --dump-symbols --check-only)
set(CASE_EXPECTED_DIAGNOSTIC [==[simp: unknown option: --dump-symbols]==])
