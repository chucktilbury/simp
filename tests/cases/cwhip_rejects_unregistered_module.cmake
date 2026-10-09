set(CASE_NAME cwhip_rejects_unregistered_module)
set(CASE_FIXTURE negative_import_unregistered.cw)
set(CASE_EXPECTED_DIAGNOSTIC [==[module 'missing_module' was not found in any package module root]==])
