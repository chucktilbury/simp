set(CASE_NAME simp_rejects_reopening_imported_namespace)
set(CASE_FIXTURE negative_import_reopen.simp)
set(CASE_EXPECTED_DIAGNOSTIC [==[import alias 'Net' conflicts with a local declaration]==])
set(CASE_MODULE_REGISTRY [==[ON]==])
