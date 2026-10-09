set(CASE_NAME cwhip_rejects_reopening_imported_namespace)
set(CASE_FIXTURE negative_import_reopen.cw)
set(CASE_EXPECTED_DIAGNOSTIC [==[import alias 'Net' conflicts with a local declaration]==])
set(CASE_MODULE_PACKAGES ON)
