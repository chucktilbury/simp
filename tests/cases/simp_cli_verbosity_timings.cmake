set(CASE_NAME simp_cli_verbosity_timings)
set(CASE_FIXTURE positive_integer_output.simp)
set(CASE_NO_RUN [==[ON]==])
set(CASE_ARGUMENTS -v -v -v --check-only)
set(CASE_EXPECT_COMPILE_OUTPUT
    [==[\[paths\] prelude source: ]==]
    [==[\[timing\] semantic analysis: [0-9.]+ ms]==])
