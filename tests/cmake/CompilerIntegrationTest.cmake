if(NOT DEFINED COMPILER OR NOT DEFINED SOURCE OR NOT DEFINED OUTPUT)
    message(FATAL_ERROR "COMPILER, SOURCE, and OUTPUT are required")
endif()

set(package_arguments)
set(package_command "${COMPILER}")
if(DEFINED PACKAGE_FIXTURE)
    set(package_search_root "${OUTPUT}.packages")
    file(MAKE_DIRECTORY "${package_search_root}")
    file(COPY "${PACKAGE_FIXTURE}/" DESTINATION "${package_search_root}")
    set(PACKAGE_SEARCH_ROOT "${package_search_root}")
endif()
if(DEFINED PACKAGE_SEARCH_ROOT)
    if(DEFINED PACKAGE_ENVIRONMENT)
        set(package_command "${CMAKE_COMMAND}" -E env
            "SIMP_PACKAGE_PATH=${PACKAGE_SEARCH_ROOT}" "${COMPILER}")
    else()
        list(APPEND package_arguments --package-path "${PACKAGE_SEARCH_ROOT}")
    endif()
endif()
if(DEFINED PACKAGE_NATIVE_SOURCE)
    if(NOT DEFINED CLANG OR NOT DEFINED AR OR NOT DEFINED PACKAGE_NATIVE_LIBRARY)
        message(FATAL_ERROR "Package native builds require CLANG, AR, and a library name")
    endif()
    set(native_source "${PACKAGE_SEARCH_ROOT}/${PACKAGE_NATIVE_SOURCE}")
    get_filename_component(native_source_directory "${native_source}" DIRECTORY)
    get_filename_component(package_version_directory "${native_source_directory}" DIRECTORY)
    set(native_library_directory "${package_version_directory}/lib")
    file(MAKE_DIRECTORY "${native_library_directory}")
    set(native_object "${OUTPUT}.native.o")
    execute_process(
        COMMAND "${CLANG}" -c "${native_source}" -o "${native_object}"
        RESULT_VARIABLE native_compile_result
        OUTPUT_VARIABLE native_compile_stdout
        ERROR_VARIABLE native_compile_stderr
    )
    if(NOT native_compile_result EQUAL 0)
        message(FATAL_ERROR "Could not compile package native shim:\n${native_compile_stderr}")
    endif()
    execute_process(
        COMMAND "${AR}" rcs
            "${native_library_directory}/lib${PACKAGE_NATIVE_LIBRARY}.a"
            "${native_object}"
        RESULT_VARIABLE native_archive_result
        OUTPUT_VARIABLE native_archive_stdout
        ERROR_VARIABLE native_archive_stderr
    )
    if(NOT native_archive_result EQUAL 0)
        message(FATAL_ERROR "Could not archive package native shim:\n${native_archive_stderr}")
    endif()
endif()

if(DEFINED EXPECTED_DIAGNOSTIC)
    execute_process(
        COMMAND ${package_command} ${package_arguments} "${SOURCE}" -o "${OUTPUT}"
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

if(NOT DEFINED IR_OUTPUT OR NOT DEFINED EXPECTED_OUTPUT_FILE)
    message(FATAL_ERROR "IR_OUTPUT and EXPECTED_OUTPUT_FILE are required for run mode")
endif()
if(NOT EXISTS "${EXPECTED_OUTPUT_FILE}")
    message(FATAL_ERROR "Missing output expectation: ${EXPECTED_OUTPUT_FILE}")
endif()
file(READ "${EXPECTED_OUTPUT_FILE}" EXPECTED_OUTPUT)
if(DEFINED COMPILE_ONLY)
    set(package_object "${OUTPUT}.o")
    execute_process(
        COMMAND ${package_command} ${package_arguments} -c "${SOURCE}" -o "${package_object}"
        RESULT_VARIABLE compile_result
        OUTPUT_VARIABLE compile_stdout
        ERROR_VARIABLE compile_stderr
    )
    if(NOT compile_result EQUAL 0)
        message(FATAL_ERROR "Package compile-only failed (${compile_result}):\n${compile_stderr}")
    endif()
    if(NOT EXISTS "${package_object}.simp-link")
        message(FATAL_ERROR "Package compile-only did not write its link sidecar")
    endif()
    execute_process(
        COMMAND ${package_command} "${package_object}" -o "${OUTPUT}"
        RESULT_VARIABLE link_result
        OUTPUT_VARIABLE link_stdout
        ERROR_VARIABLE link_stderr
    )
    if(NOT link_result EQUAL 0)
        message(FATAL_ERROR "Package object relink failed (${link_result}):\n${link_stderr}")
    endif()
    execute_process(
        COMMAND "${OUTPUT}"
        RESULT_VARIABLE run_result
        OUTPUT_VARIABLE program_output
        ERROR_VARIABLE run_stderr
    )
    if(NOT run_result EQUAL 0 OR NOT program_output STREQUAL EXPECTED_OUTPUT)
        message(FATAL_ERROR
            "Package object program result was '${program_output}': ${run_stderr}")
    endif()
    return()
endif()
execute_process(
    COMMAND ${package_command} ${package_arguments} "${SOURCE}" -o "${OUTPUT}" --emit-llvm "${IR_OUTPUT}"
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
        if(DEFINED EXPECT_RUNTIME_FRAMES)
            string(REPLACE "|" ";" expected_frames "${EXPECT_RUNTIME_FRAMES}")
            set(previous_frame_position -1)
            foreach(expected_frame IN LISTS expected_frames)
                string(FIND "${run_stderr}" "${expected_frame}" frame_position)
                if(frame_position LESS 0 OR frame_position LESS_EQUAL previous_frame_position)
                    message(FATAL_ERROR
                        "Expected ordered runtime frame '${expected_frame}', got:\n${run_stderr}")
                endif()
                string(SUBSTRING "${run_stderr}" ${frame_position} -1 frame_tail)
                string(FIND "${frame_tail}" "\n" frame_line_end)
                if(frame_line_end LESS 0)
                    set(frame_line "${frame_tail}")
                else()
                    string(SUBSTRING "${frame_tail}" 0 ${frame_line_end} frame_line)
                endif()
                if(NOT frame_line MATCHES "\\([^)]*:[0-9]+:[0-9]+\\)$")
                    message(FATAL_ERROR
                        "Runtime frame '${expected_frame}' has no source location: ${frame_line}")
                endif()
                set(previous_frame_position "${frame_position}")
            endforeach()
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
if(DEFINED EXPECT_EMPTY_RUNTIME_STDERR AND NOT run_stderr STREQUAL "")
    message(FATAL_ERROR "Expected no runtime diagnostic on stderr, got:\n${run_stderr}")
endif()
