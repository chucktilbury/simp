if(NOT DEFINED COMPILER OR NOT DEFINED SOURCE OR
   NOT DEFINED WORKING_DIRECTORY OR NOT DEFINED OUTPUT_NAME OR
   NOT DEFINED IR_OUTPUT OR NOT DEFINED EXPECTED_OUTPUT)
    message(FATAL_ERROR
        "COMPILER, SOURCE, WORKING_DIRECTORY, OUTPUT_NAME, IR_OUTPUT, and EXPECTED_OUTPUT are required")
endif()

file(MAKE_DIRECTORY "${WORKING_DIRECTORY}")
set(executable "${WORKING_DIRECTORY}/${OUTPUT_NAME}")
file(REMOVE "${executable}")

execute_process(
    COMMAND "${COMPILER}" "${SOURCE}"
    WORKING_DIRECTORY "${WORKING_DIRECTORY}"
    RESULT_VARIABLE compile_result
    OUTPUT_VARIABLE compile_stdout
    ERROR_VARIABLE compile_stderr
)
if(NOT compile_result EQUAL 0)
    message(FATAL_ERROR "Default-output compilation failed:\n${compile_stderr}")
endif()
if(NOT EXISTS "${executable}")
    message(FATAL_ERROR "Compiler did not create ${executable} in its working directory")
endif()
execute_process(
    COMMAND "${executable}"
    RESULT_VARIABLE run_result
    OUTPUT_VARIABLE program_output
    ERROR_VARIABLE run_stderr
)
if(NOT run_result EQUAL 0 OR NOT program_output STREQUAL EXPECTED_OUTPUT)
    message(FATAL_ERROR
        "Default-output executable failed (${run_result}): '${program_output}'\n${run_stderr}")
endif()
file(REMOVE "${executable}")

execute_process(
    COMMAND "${COMPILER}" "${SOURCE}" --emit-llvm "${IR_OUTPUT}"
    WORKING_DIRECTORY "${WORKING_DIRECTORY}"
    RESULT_VARIABLE emit_result
    OUTPUT_VARIABLE emit_stdout
    ERROR_VARIABLE emit_stderr
)
if(NOT emit_result EQUAL 0)
    message(FATAL_ERROR "Compilation with --emit-llvm failed:\n${emit_stderr}")
endif()
if(NOT EXISTS "${executable}" OR NOT EXISTS "${IR_OUTPUT}")
    message(FATAL_ERROR
        "Compiler did not create the default executable and requested LLVM IR")
endif()
execute_process(
    COMMAND "${executable}"
    RESULT_VARIABLE emitted_run_result
    OUTPUT_VARIABLE emitted_program_output
    ERROR_VARIABLE emitted_run_stderr
)
if(NOT emitted_run_result EQUAL 0 OR
   NOT emitted_program_output STREQUAL EXPECTED_OUTPUT)
    message(FATAL_ERROR
        "Executable built with --emit-llvm failed (${emitted_run_result}): '${emitted_program_output}'\n${emitted_run_stderr}")
endif()
file(REMOVE "${executable}" "${IR_OUTPUT}")
