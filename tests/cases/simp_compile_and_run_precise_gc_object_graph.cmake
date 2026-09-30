set(CASE_NAME simp_compile_and_run_precise_gc_object_graph)
set(CASE_FIXTURE positive_gc_object_graph.simp)
set(CASE_EXPECTED_OUTPUT_FILE "${CMAKE_CURRENT_LIST_DIR}/simp_compile_and_run_precise_gc_object_graph.stdout")
set(CASE_REQUIRE_GC_ROOTS [==[ON]==])
