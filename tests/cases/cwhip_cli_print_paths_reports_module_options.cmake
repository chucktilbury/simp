set(CASE_NAME cwhip_cli_print_paths_reports_module_options)
set(CASE_FIXTURE positive_integer_output.cw)
set(CASE_MODULE_ROOT_SOURCE cli)
set(CASE_MODULE_DECOY decoy)
set(CASE_NO_RUN [==[ON]==])
set(CASE_ARGUMENTS --print-paths)
set(CASE_EXPECT_COMPILE_OUTPUT
    [==[project package root: <PROJECT_DIR>/modules \(project <project-root>/modules\)]==]
    [==[project lock: <PROJECT_DIR>/cwhip-pkg\.lock \[not found\] \(project lock\)]==]
    [==[package root \[1\]: <MODULE_ROOT> \(-M/--module-dir\)]==]
    [==[package root \[2\]: <PROJECT_DIR>/modules \(project <project-root>/modules\)]==]
    [==[package root \[3\]: <WORK_DIR>/env-modules \(CWHIP_MODULE_DIR\)]==]
    [==[package root \[4\]: <WORK_DIR>/xdg/cwhip/modules \[not found\] \(XDG_CONFIG_HOME/cwhip/modules\)]==]
    [==[package root \[5\]: <WORK_DIR>/stdlib-decoy \(CWHIP_STDLIB_MODULE_DIR\)]==])
set(CASE_REJECT_COMPILE_OUTPUT [==[warning]==])
