set(CASE_NAME simp_rejects_import_in_function)
set(CASE_FIXTURE negative_import_function_body.simp)
set(CASE_EXPECTED_DIAGNOSTIC [==['import' is only allowed at top level]==])
set(CASE_MODULE_PACKAGES ON)
