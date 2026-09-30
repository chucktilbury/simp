set(CASE_NAME simp_rejects_private_field_access)
set(CASE_FIXTURE negative_private_member.simp)
set(CASE_EXPECTED_DIAGNOSTIC [==[field 'value' is not accessible]==])
