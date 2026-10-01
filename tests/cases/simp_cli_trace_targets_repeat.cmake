set(CASE_NAME simp_cli_trace_targets_repeat)
set(CASE_FIXTURE positive_integer_output.simp)
set(CASE_NO_RUN [==[ON]==])
set(CASE_ARGUMENTS -t scanner --trace=AST -t scanner --check-only)
set(CASE_EXPECT_COMPILE_OUTPUT [==[\[scanner\] 'start' "start"]==] [==[Program
]==])
set(CASE_REJECT_COMPILE_OUTPUT [==[\[parser\]]==] [==[Symbols:]==])
