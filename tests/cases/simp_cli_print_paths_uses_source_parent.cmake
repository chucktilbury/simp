set(CASE_NAME simp_cli_print_paths_uses_source_parent)
set(CASE_FIXTURE positive_integer_output.simp)
set(CASE_MODULE_ROOT_SOURCE default)
set(CASE_NO_RUN [==[ON]==])
set(CASE_ARGUMENTS --print-paths)
set(CASE_EXPECT_COMPILE_OUTPUT
    [==[executable: [^
]+/simp
prefix: ]==]
    [==[runtime library: [^
]+/simp/libsimp_runtime\.a
include directory: ]==]
    [==[builtin source: [^
]+/simp/builtin/String\.simp
project root: <PROJECT_DIR> \(parent of first source input\)]==]
    [==[project package root: <PROJECT_DIR>/modules \(project <project-root>/modules\)]==]
    [==[project lock: <PROJECT_DIR>/simpkg\.lock \[not found\] \(project lock\)]==]
    [==[package root \[1\]: <PROJECT_DIR>/modules \(project <project-root>/modules\)]==]
    [==[package root \[2\]: <WORK_DIR>/xdg/simp/modules \[not found\] \(XDG_CONFIG_HOME/simp/modules\)]==]
    [==[package root \[3\]: [^
]+/simp/modules \(executable-relative\)
clang: ]==])
set(CASE_REJECT_COMPILE_OUTPUT [==[simp: built]==] [==[warning]==])
