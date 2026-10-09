set(CASE_NAME cwhip_cli_rejects_removed_dump_symbols)
set(CASE_FIXTURE positive_integer_output.cw)
set(CASE_ARGUMENTS --dump-symbols --check-only)
set(CASE_EXPECTED_DIAGNOSTIC [==[cwhip: unknown option: --dump-symbols]==])
