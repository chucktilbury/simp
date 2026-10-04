set(CASE_NAME simp_rejects_private_imported_class_integer_enum)
set(CASE_FIXTURE negative_imported_class_integer_enum_access.simp)
set(CASE_EXPECTED_DIAGNOSTIC [==[enum member 'SECRET' is not accessible]==])
set(CASE_MODULE_ROOT packages)
