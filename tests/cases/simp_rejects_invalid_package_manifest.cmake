set(CASE_NAME simp_rejects_invalid_package_manifest)
set(CASE_FIXTURE negative_package_invalid_manifest.simp)
set(CASE_PACKAGE_FIXTURE "${CMAKE_CURRENT_LIST_DIR}/../functional/modules/packages")
set(CASE_EXPECTED_DIAGNOSTIC "unknown key 'unknown-option' in \\[package\\]")
