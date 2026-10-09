set(CASE_NAME cwhip_rejects_private_field_access)
set(CASE_FIXTURE negative_private_member.cw)
set(CASE_EXPECTED_DIAGNOSTIC [==[field 'value' is not accessible]==])
