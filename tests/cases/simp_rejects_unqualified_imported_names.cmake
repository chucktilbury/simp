set(CASE_NAME simp_rejects_unqualified_imported_names)
set(CASE_FIXTURE negative_import_unqualified.simp)
set(CASE_EXPECTED_DIAGNOSTIC [==[undefined variable 'Network' or field]==])
set(CASE_MODULE_PACKAGES ON)
