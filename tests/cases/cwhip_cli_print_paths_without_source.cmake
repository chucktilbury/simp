set(CASE_NAME cwhip_cli_print_paths_without_source)
set(CASE_FIXTURE positive_integer_output.cw)
set(CASE_NO_SOURCE [==[ON]==])
set(CASE_NO_RUN [==[ON]==])
set(CASE_ARGUMENTS --print-paths)
set(CASE_EXPECT_COMPILE_OUTPUT
[==[project package root: <WORK_DIR>/modules \[not found\] \(project <project-root>/modules\)
project lock: <WORK_DIR>/cwhip-pkg\.lock \[not found\] \(project lock\)
package root \[1\]: <WORK_DIR>/modules \[not found\] \(project <project-root>/modules\)
package root \[2\]: <WORK_DIR>/xdg/cwhip/modules \[not found\] \(XDG_CONFIG_HOME/cwhip/modules\)
package root \[3\]: [^
]+/cwhip/modules \(executable-relative\)]==])
