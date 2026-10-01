set(CASE_NAME simp_rejects_missing_package_version)
set(CASE_FIXTURE negative_package_missing_version.simp)
set(CASE_MODULE_ROOT packages)
set(CASE_EXPECTED_DIAGNOSTIC "package 'shared' requires version '9.0.0', but it is not installed")
