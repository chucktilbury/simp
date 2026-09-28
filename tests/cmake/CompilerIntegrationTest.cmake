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
if(NOT EXISTS "${IR_OUTPUT}")
    message(FATAL_ERROR "Compiler did not write requested LLVM IR: ${IR_OUTPUT}")
endif()
file(READ "${IR_OUTPUT}" ir_text)
if(NOT ir_text MATCHES "define i32 @main")
    message(FATAL_ERROR "Emitted LLVM IR does not define main")
endif()

execute_process(
    COMMAND "${OUTPUT}"
    RESULT_VARIABLE run_result
    OUTPUT_VARIABLE program_output
    ERROR_VARIABLE run_stderr
)
if(NOT run_result EQUAL 0)
    message(FATAL_ERROR "Compiled Simple program exited ${run_result}:\n${run_stderr}")
endif()
if(NOT program_output STREQUAL EXPECTED_OUTPUT)
    message(FATAL_ERROR "Expected output '${EXPECTED_OUTPUT}', got '${program_output}'")
endif()
