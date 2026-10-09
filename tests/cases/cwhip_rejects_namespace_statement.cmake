set(CASE_NAME cwhip_rejects_namespace_statement)
set(CASE_FIXTURE negative_namespace_statement.cw)
set(CASE_EXPECTED_DIAGNOSTIC [==[namespace body may contain only namespace and class declarations]==])
