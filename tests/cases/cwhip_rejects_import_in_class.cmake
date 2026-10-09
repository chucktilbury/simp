set(CASE_NAME cwhip_rejects_import_in_class)
set(CASE_FIXTURE negative_import_class_body.cw)
set(CASE_EXPECTED_DIAGNOSTIC [==['import' is only allowed at top level]==])
set(CASE_MODULE_PACKAGES ON)
