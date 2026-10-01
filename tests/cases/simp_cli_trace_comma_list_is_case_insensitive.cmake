set(CASE_NAME simp_cli_trace_comma_list_is_case_insensitive)
set(CASE_FIXTURE positive_integer_output.simp)
set(CASE_NO_RUN [==[ON]==])
set(CASE_ARGUMENTS -t Parser,SYMBOLS --check-only)
set(CASE_EXPECT_COMPILE_OUTPUT
    [==[\[parser\] enter start block]==]
    [==[Symbols:
  String text]==])
set(CASE_REJECT_COMPILE_OUTPUT [==[\[scanner\]]==] [==[Program
]==])
