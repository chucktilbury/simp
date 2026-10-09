set(CASE_NAME cwhip_compile_and_run_threads)
set(CASE_FIXTURE positive_threads.cw)
set(CASE_EXPECTED_OUTPUT_FILE "${CMAKE_CURRENT_LIST_DIR}/cwhip_compile_and_run_threads.stdout")
set(CASE_REQUIRE_GC_ROOTS [==[ON]==])
