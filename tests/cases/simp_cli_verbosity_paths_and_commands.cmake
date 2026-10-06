set(CASE_NAME simp_cli_verbosity_paths_and_commands)
set(CASE_FIXTURE positive_integer_output.simp)
set(CASE_MODULE_ROOT_SOURCE default)
set(CASE_NO_RUN [==[ON]==])
set(CASE_ARGUMENTS -vv)
set(CASE_EXPECT_COMPILE_OUTPUT
    [==[\[verbose\] compile and link with clang]==]
    [==[\[paths\] project root: <PROJECT_DIR> \(parent of first source input\)]==]
    [==[\[paths\] project package root: <PROJECT_DIR>/modules \(project <project-root>/modules\)]==]
    [==[\[paths\] project lock: <PROJECT_DIR>/simpkg\.lock \[not found\] \(project lock\)]==]
    [==[\[paths\] runtime library: [^
]+/simp/libsimp_runtime\.a]==]
    [==[\[command\] '[^']+' '-Wno-override-module' [^
]+/simp/libsimp_runtime\.a' '-lm' '-o' ']==])
set(CASE_REJECT_COMPILE_OUTPUT [==[\[timing\]]==])
