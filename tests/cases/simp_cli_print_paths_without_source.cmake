set(CASE_NAME simp_cli_print_paths_without_source)
set(CASE_FIXTURE positive_integer_output.simp)
set(CASE_NO_SOURCE [==[ON]==])
set(CASE_NO_RUN [==[ON]==])
set(CASE_ARGUMENTS --print-paths)
set(CASE_EXPECT_COMPILE_OUTPUT
[==[project package root: <WORK_DIR>/modules \[not found\] \(project <project-root>/modules\)
project lock: <WORK_DIR>/simpkg\.lock \[not found\] \(project lock\)
package root \[1\]: <WORK_DIR>/modules \[not found\] \(project <project-root>/modules\)
package root \[2\]: <WORK_DIR>/xdg/simp/modules \[not found\] \(XDG_CONFIG_HOME/simp/modules\)
package root \[3\]: [^
]+/simp/modules \(executable-relative\)]==])
