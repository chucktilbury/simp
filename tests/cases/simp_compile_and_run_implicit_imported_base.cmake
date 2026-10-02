set(CASE_NAME simp_compile_and_run_implicit_imported_base)
set(CASE_FIXTURE positive_implicit_imported_base.simp)
set(CASE_MODULE_REGISTRY ON)
set(CASE_EXPECT_WARNING [==[simp: warning: module 'greeting' was resolved through the deprecated module registry]==])
set(CASE_EXPECTED_OUTPUT [==[42
42
]==])
