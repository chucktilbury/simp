set(CASE_NAME simp_cli_print_paths_reports_module_options)
set(CASE_FIXTURE positive_integer_output.simp)
set(CASE_MODULE_ROOT_SOURCE cli)
set(CASE_MODULE_DECOY decoy)
set(CASE_NO_RUN [==[ON]==])
set(CASE_ARGUMENTS --print-paths --package-path compat-a:compat-b)
set(CASE_EXPECT_COMPILE_OUTPUT
    [==[project module root: <MODULE_ROOT> \(-M/--module-dir\)]==]
    [==[compatibility package root: <WORK_DIR>/compat-a \[not found\] \(--package-path, deprecated\)
compatibility package root: <WORK_DIR>/compat-b \[not found\] \(--package-path, deprecated\)]==])
set(CASE_REJECT_COMPILE_OUTPUT [==[warning]==])
