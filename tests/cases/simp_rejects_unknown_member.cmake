set(CASE_NAME simp_rejects_unknown_member)
set(CASE_FIXTURE negative_unknown_member.simp)
set(CASE_EXPECTED_DIAGNOSTIC [==[class 'Counter' has no field 'missing']==])
