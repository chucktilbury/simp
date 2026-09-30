set(CASE_NAME simp_rejects_finalizer_allocation)
set(CASE_FIXTURE negative_finalizer_allocation.simp)
set(CASE_EXPECTED_OUTPUT_FILE "${CMAKE_CURRENT_LIST_DIR}/simp_rejects_finalizer_allocation.stdout")
set(CASE_EXPECT_RUNTIME_FAILURE [==[ON]==])
