set(CASE_NAME simp_rejects_removed_package_path)
set(CASE_FIXTURE positive_integer_output.simp)
set(CASE_NO_SOURCE ON)
set(CASE_NO_RUN ON)
set(CASE_ARGUMENTS --package-path old-modules)
set(CASE_EXPECTED_DIAGNOSTIC
    [==[simp: --package-path is no longer supported; use -M/--module-dir]==])
