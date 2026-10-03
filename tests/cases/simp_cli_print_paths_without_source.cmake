set(CASE_NAME simp_cli_print_paths_without_source)
set(CASE_FIXTURE positive_integer_output.simp)
set(CASE_NO_SOURCE [==[ON]==])
set(CASE_NO_RUN [==[ON]==])
set(CASE_ARGUMENTS --print-paths)
set(CASE_EXPECT_COMPILE_OUTPUT
[==[project module root: <WORK_DIR>/modules \[not found\] \(default <project-root>/modules\)
module selection file: <WORK_DIR>/modules/modules\.toml \[not found\] \(project module selection\)]==])
