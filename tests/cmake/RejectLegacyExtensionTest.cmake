file(MAKE_DIRECTORY "${WORK_DIR}")
set(source "${WORK_DIR}/legacy.simp")
file(WRITE "${source}" "start {}\n")

execute_process(
    COMMAND "${COMPILER}" "${source}" --check-only
    RESULT_VARIABLE result
    OUTPUT_VARIABLE output
    ERROR_VARIABLE error
)
if(result EQUAL 0 OR NOT error MATCHES "unsupported input file type")
    message(FATAL_ERROR
        "The compiler must reject the legacy source suffix (result=${result}): ${error}${output}")
endif()
