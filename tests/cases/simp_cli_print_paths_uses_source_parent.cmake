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
project root: <PROJECT_DIR> \(parent of first source input\)
project module root: <PROJECT_DIR>/modules \(default <project-root>/modules\)
module selection file: <PROJECT_DIR>/modules/modules\.toml \[not found\] \(project module selection\)
standard modules: [^
]+/simp/modules \(executable-relative\)
compatibility package roots: <none>
module registry: <WORK_DIR>/simp-modules.tsv \[not found\] \(default ./simp-modules.tsv, deprecated\)
clang: ]==])
set(CASE_REJECT_COMPILE_OUTPUT [==[simp: built]==] [==[warning]==])
