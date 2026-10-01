set(CASE_NAME simp_rejects_invalid_package_manifest)
set(CASE_FIXTURE negative_package_invalid_manifest.simp)
set(CASE_MODULE_ROOT packages)
set(CASE_EXPECTED_DIAGNOSTIC "unknown key 'unknown-option' in \\[package\\]")
