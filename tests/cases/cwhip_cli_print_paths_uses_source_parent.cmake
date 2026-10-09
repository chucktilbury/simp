set(CASE_NAME cwhip_cli_print_paths_uses_source_parent)
set(CASE_FIXTURE positive_integer_output.cw)
set(CASE_MODULE_ROOT_SOURCE default)
set(CASE_NO_RUN [==[ON]==])
set(CASE_ARGUMENTS --print-paths)
set(CASE_EXPECT_COMPILE_OUTPUT
    [==[executable: [^
]+/cwhip
prefix: ]==]
    [==[runtime library: [^
]+/cwhip/libcwhip_runtime\.a
include directory: ]==]
    [==[builtin source: [^
]+/cwhip/builtin/String\.cw
project root: <PROJECT_DIR> \(parent of first source input\)]==]
    [==[project package root: <PROJECT_DIR>/modules \(project <project-root>/modules\)]==]
    [==[project lock: <PROJECT_DIR>/cwhip-pkg\.lock \[not found\] \(project lock\)]==]
    [==[package root \[1\]: <PROJECT_DIR>/modules \(project <project-root>/modules\)]==]
    [==[package root \[2\]: <WORK_DIR>/xdg/cwhip/modules \[not found\] \(XDG_CONFIG_HOME/cwhip/modules\)]==]
    [==[package root \[3\]: [^
]+/cwhip/modules \(executable-relative\)
clang: ]==])
set(CASE_REJECT_COMPILE_OUTPUT [==[cwhip: built]==] [==[warning]==])
