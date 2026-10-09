set(CASE_NAME cwhip_rejects_use_after_destructor)
set(CASE_FIXTURE negative_use_after_destructor.cw)
set(CASE_EXPECTED_OUTPUT_FILE "${CMAKE_CURRENT_LIST_DIR}/cwhip_rejects_use_after_destructor.stdout")
set(CASE_EXPECT_RUNTIME_FAILURE [==[ON]==])
