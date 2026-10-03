set(CASE_NAME simp_module_selection_cli_root)
set(CASE_FIXTURE positive_package_version_pins.simp)
set(CASE_MODULE_ROOT packages)
set(CASE_MODULE_POLICY policy/version-order.toml)
set(CASE_MODULE_ROOT_SOURCE cli)
set(CASE_ARGUMENTS -v)
set(CASE_EXPECT_COMPILE_OUTPUT
    [==[\[verbose\] selected module shared@1\.0\.0 from "<MODULE_ROOT>/shared/1\.0\.0/src/shared\.simp"]==]
    [==[\[verbose\] selected module alpha@1\.0\.0 from "<MODULE_ROOT>/alpha/1\.0\.0/src/alpha\.simp"]==])
set(CASE_EXPECTED_OUTPUT_FILE "${CMAKE_CURRENT_LIST_DIR}/simp_package_version_pins.stdout")
