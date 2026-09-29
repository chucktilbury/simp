if(NOT DEFINED COMPILER OR NOT DEFINED SOURCE OR NOT DEFINED OUTPUT)
    message(FATAL_ERROR "COMPILER, SOURCE, and OUTPUT are required")
endif()

if(DEFINED EXPECTED_DIAGNOSTIC)
    execute_process(
        COMMAND "${COMPILER}" "${SOURCE}" -o "${OUTPUT}"
        RESULT_VARIABLE result
        OUTPUT_VARIABLE stdout
        ERROR_VARIABLE stderr
    )
    if(result EQUAL 0)
        message(FATAL_ERROR "Expected compilation to fail, but it succeeded")
    endif()
    if(NOT stderr MATCHES "${EXPECTED_DIAGNOSTIC}")
        message(FATAL_ERROR "Expected diagnostic '${EXPECTED_DIAGNOSTIC}', got:\n${stderr}")
    endif()
    return()
endif()

if(NOT DEFINED IR_OUTPUT OR NOT DEFINED EXPECTED_OUTPUT)
    message(FATAL_ERROR "IR_OUTPUT and EXPECTED_OUTPUT are required for run mode")
endif()
execute_process(
    COMMAND "${COMPILER}" "${SOURCE}" -o "${OUTPUT}" --emit-llvm "${IR_OUTPUT}"
    RESULT_VARIABLE compile_result
    OUTPUT_VARIABLE compile_stdout
    ERROR_VARIABLE compile_stderr
)
if(NOT compile_result EQUAL 0)
    message(FATAL_ERROR "Compilation failed (${compile_result}):\n${compile_stderr}")
endif()
if(DEFINED EXPECT_WARNING AND NOT compile_stderr MATCHES "${EXPECT_WARNING}")
    message(FATAL_ERROR "Expected compiler warning '${EXPECT_WARNING}', got:\n${compile_stderr}")
endif()
if(NOT EXISTS "${IR_OUTPUT}")
    message(FATAL_ERROR "Compiler did not write requested LLVM IR: ${IR_OUTPUT}")
endif()
file(READ "${IR_OUTPUT}" ir_text)
if(NOT ir_text MATCHES "define i32 @main")
    message(FATAL_ERROR "Emitted LLVM IR does not define main")
endif()
if(DEFINED REQUIRE_GC_ROOTS)
    foreach(required_gc_symbol IN ITEMS
            "@simp_gc_push_tagged_or_abort" "@simp_gc_pop_or_abort" "@simp_gc_alloc")
        if(NOT ir_text MATCHES "${required_gc_symbol}")
            message(FATAL_ERROR "Emitted LLVM IR is missing ${required_gc_symbol}")
        endif()
    endforeach()
    string(REGEX MATCHALL "call void @simp_gc_push_or_abort\\(" root_push_calls "${ir_text}")
    string(REGEX MATCHALL "call void @simp_gc_push_tagged_or_abort\\("
           tagged_root_push_calls "${ir_text}")
    string(REGEX MATCHALL "call void @simp_gc_pop_or_abort\\(" root_pop_calls "${ir_text}")
    list(LENGTH root_push_calls root_push_count)
    list(LENGTH tagged_root_push_calls tagged_root_push_count)
    list(LENGTH root_pop_calls root_pop_count)
    math(EXPR root_push_count "${root_push_count} + ${tagged_root_push_count}")
    if(root_push_count EQUAL 0 OR NOT root_push_count EQUAL root_pop_count)
        message(FATAL_ERROR
            "Generated functions have unbalanced root-frame lifecycle (${root_push_count} pushes, ${root_pop_count} pops)")
    endif()
endif()
if(DEFINED REQUIRE_VIRTUAL_DISPATCH)
    if(NOT ir_text MATCHES "call i32 %t[0-9]+\\(")
        message(FATAL_ERROR "Generated IR does not call a method through a loaded dispatch slot")
    endif()
    if(NOT ir_text MATCHES "call i32 @simp.Grandchild.read\\(")
        message(FATAL_ERROR "Most-derived override is missing from generated dispatch thunks")
    endif()
endif()

execute_process(
    COMMAND "${OUTPUT}"
    RESULT_VARIABLE run_result
    OUTPUT_VARIABLE program_output
    ERROR_VARIABLE run_stderr
)
if(NOT run_result EQUAL 0)
    if(DEFINED EXPECT_RUNTIME_FAILURE)
        if(DEFINED EXPECT_RUNTIME_DIAGNOSTIC AND
           NOT run_stderr MATCHES "${EXPECT_RUNTIME_DIAGNOSTIC}")
            message(FATAL_ERROR
                "Expected runtime diagnostic '${EXPECT_RUNTIME_DIAGNOSTIC}', got:\n${run_stderr}")
        endif()
        if(NOT program_output STREQUAL EXPECTED_OUTPUT)
            message(FATAL_ERROR
                "Expected output '${EXPECTED_OUTPUT}' before runtime failure, got '${program_output}'")
        endif()
        return()
    endif()
    message(FATAL_ERROR "Compiled Simple program exited ${run_result}:\n${run_stderr}")
endif()
if(DEFINED EXPECT_RUNTIME_FAILURE)
    message(FATAL_ERROR "Expected compiled program to fail at runtime, but it exited successfully")
endif()
if(NOT program_output STREQUAL EXPECTED_OUTPUT)
    message(FATAL_ERROR "Expected output '${EXPECTED_OUTPUT}', got '${program_output}'")
endif()
