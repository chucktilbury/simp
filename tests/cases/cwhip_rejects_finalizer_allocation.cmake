set(CASE_NAME cwhip_rejects_finalizer_allocation)
set(CASE_FIXTURE negative_finalizer_allocation.cw)
set(CASE_EXPECTED_OUTPUT_FILE "${CMAKE_CURRENT_LIST_DIR}/cwhip_rejects_finalizer_allocation.stdout")
set(CASE_EXPECT_RUNTIME_FAILURE [==[ON]==])
