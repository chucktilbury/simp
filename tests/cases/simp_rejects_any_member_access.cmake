set(CASE_NAME simp_rejects_any_member_access)
set(CASE_FIXTURE negative_any_member_access.simp)
set(CASE_EXPECTED_DIAGNOSTIC [==['any' has no members; assign it to a typed variable first to extract its value]==])
