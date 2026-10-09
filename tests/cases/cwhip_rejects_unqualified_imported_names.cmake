set(CASE_NAME cwhip_rejects_unqualified_imported_names)
set(CASE_FIXTURE negative_import_unqualified.cw)
set(CASE_EXPECTED_DIAGNOSTIC [==[undefined variable 'Network' or field]==])
set(CASE_MODULE_PACKAGES ON)
