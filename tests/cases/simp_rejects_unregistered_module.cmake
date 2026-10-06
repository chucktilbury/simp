set(CASE_NAME simp_rejects_unregistered_module)
set(CASE_FIXTURE negative_import_unregistered.simp)
set(CASE_EXPECTED_DIAGNOSTIC [==[module 'missing_module' was not found in any package module root]==])
