set(CASE_NAME cwhip_rejects_unknown_member)
set(CASE_FIXTURE negative_unknown_member.cw)
set(CASE_EXPECTED_DIAGNOSTIC [==[class 'Counter' has no field 'missing']==])
