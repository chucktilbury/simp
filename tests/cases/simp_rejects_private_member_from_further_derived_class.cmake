set(CASE_NAME simp_rejects_private_member_from_further_derived_class)
set(CASE_FIXTURE negative_private_inheritance_context.simp)
set(CASE_EXPECTED_DIAGNOSTIC [==[field 'value' is not accessible in this class]==])
