set(CASE_NAME cwhip_compile_and_run_explicit_destructor)
set(CASE_FIXTURE positive_explicit_destructor.cw)
set(CASE_EXPECTED_OUTPUT_FILE "${CMAKE_CURRENT_LIST_DIR}/cwhip_compile_and_run_explicit_destructor.stdout")
set(CASE_REJECT_COMPILE_OUTPUT [==[warning: explicit destructor call]==])
