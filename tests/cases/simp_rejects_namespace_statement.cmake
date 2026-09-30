set(CASE_NAME simp_rejects_namespace_statement)
set(CASE_FIXTURE negative_namespace_statement.simp)
set(CASE_EXPECTED_DIAGNOSTIC [==[namespace body may contain only namespace and class declarations]==])
