if(NOT DEFINED COMPILER OR NOT DEFINED CLANG OR NOT DEFINED AR OR
   NOT DEFINED WORK_DIR OR NOT DEFINED FIXTURES)
    message(FATAL_ERROR "COMPILER, CLANG, AR, WORK_DIR, and FIXTURES are required")
endif()

if(NOT DEFINED CASE)
    set(CASE all)
endif()
set(work "${WORK_DIR}/multi input tests/${CASE}")
file(MAKE_DIRECTORY "${work}")
set(helper "${work}/helper.simp")
set(main "${work}/main.simp")
set(executable "${work}/combined executable")
set(ir "${work}/combined.ll")
foreach(fixture IN ITEMS helper.simp main.simp native.simp native.c module.simp
        simp-modules.tsv import.simp second-start.simp no-start.simp
        duplicate-a.simp duplicate-b.simp)
    configure_file("${FIXTURES}/${fixture}" "${work}/${fixture}" COPYONLY)
endforeach()

if(CASE STREQUAL "all" OR CASE STREQUAL "combined")
execute_process(
    COMMAND "${COMPILER}" "${helper}" "${main}" --emit-llvm "${ir}" -o "${executable}"
    RESULT_VARIABLE result OUTPUT_VARIABLE stdout ERROR_VARIABLE stderr
)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "Multi-source compilation failed:\n${stderr}")
endif()
if(NOT EXISTS "${ir}")
    message(FATAL_ERROR "Multi-source compilation did not write LLVM IR")
endif()
file(READ "${ir}" ir_text)
if(NOT ir_text MATCHES "define i32 @main")
    message(FATAL_ERROR "Combined LLVM IR does not contain the single program entry")
endif()
execute_process(
    COMMAND "${executable}"
    RESULT_VARIABLE result OUTPUT_VARIABLE stdout ERROR_VARIABLE stderr
)
if(NOT result EQUAL 0 OR NOT stdout STREQUAL "42\n")
    message(FATAL_ERROR "Multi-source program result was '${stdout}': ${stderr}")
endif()
endif()

if(CASE STREQUAL "all" OR CASE STREQUAL "object")
set(combined_object "${work}/combined.o")
execute_process(
    COMMAND "${COMPILER}" -c "${helper}" "${main}" -o "${combined_object}"
    RESULT_VARIABLE result OUTPUT_VARIABLE stdout ERROR_VARIABLE stderr
)
if(NOT result EQUAL 0 OR NOT EXISTS "${combined_object}")
    message(FATAL_ERROR "Compile-only failed to produce an object:\n${stderr}")
endif()
set(object_executable "${work}/object executable")
execute_process(
    COMMAND "${COMPILER}" "${combined_object}" -o "${object_executable}"
    RESULT_VARIABLE result OUTPUT_VARIABLE stdout ERROR_VARIABLE stderr
)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "Object-only link failed:\n${stderr}")
endif()
execute_process(
    COMMAND "${object_executable}"
    RESULT_VARIABLE result OUTPUT_VARIABLE stdout ERROR_VARIABLE stderr
)
if(NOT result EQUAL 0 OR NOT stdout STREQUAL "42\n")
    message(FATAL_ERROR "Object-only linked program result was '${stdout}': ${stderr}")
endif()
endif()

if(CASE STREQUAL "all" OR CASE STREQUAL "mixed" OR CASE STREQUAL "library")
set(native "${work}/native.simp")
set(c_source "${work}/native.c")
set(c_object "${work}/native.o")
set(c_obj_object "${work}/native.obj")
execute_process(
    COMMAND "${CLANG}" -c "${c_source}" -o "${c_object}"
    RESULT_VARIABLE result OUTPUT_VARIABLE stdout ERROR_VARIABLE stderr
)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "Could not compile the external test object:\n${stderr}")
endif()
configure_file("${c_object}" "${c_obj_object}" COPYONLY)
endif()
if(CASE STREQUAL "all" OR CASE STREQUAL "mixed")
set(mixed_executable "${work}/mixed executable")
execute_process(
    COMMAND "${COMPILER}" "${native}" "${c_obj_object}" -o "${mixed_executable}"
    RESULT_VARIABLE result OUTPUT_VARIABLE stdout ERROR_VARIABLE stderr
)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "Mixed source/object link failed:\n${stderr}")
endif()
execute_process(
    COMMAND "${mixed_executable}"
    RESULT_VARIABLE result OUTPUT_VARIABLE stdout ERROR_VARIABLE stderr
)
if(NOT result EQUAL 0 OR NOT stdout STREQUAL "42\n")
    message(FATAL_ERROR "Mixed source/object result was '${stdout}': ${stderr}")
endif()
endif()

if(CASE STREQUAL "all" OR CASE STREQUAL "library")
set(library_directory "${work}/libraries")
file(MAKE_DIRECTORY "${library_directory}")
set(library "${library_directory}/libmulti_test.a")
execute_process(
    COMMAND "${AR}" rcs "${library}" "${c_object}"
    RESULT_VARIABLE result OUTPUT_VARIABLE stdout ERROR_VARIABLE stderr
)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "Could not create test archive:\n${stderr}")
endif()
set(library_executable "${work}/library executable")
execute_process(
    COMMAND "${COMPILER}" "${native}" -L "${library_directory}" -l multi_test
            -o "${library_executable}"
    RESULT_VARIABLE result OUTPUT_VARIABLE stdout ERROR_VARIABLE stderr
)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "-L/-l link failed:\n${stderr}")
endif()
execute_process(
    COMMAND "${library_executable}"
    RESULT_VARIABLE result OUTPUT_VARIABLE stdout ERROR_VARIABLE stderr
)
if(NOT result EQUAL 0 OR NOT stdout STREQUAL "42\n")
    message(FATAL_ERROR "Library-linked program result was '${stdout}': ${stderr}")
endif()
endif()

if(CASE STREQUAL "all" OR CASE STREQUAL "module")
set(module_source "${work}/module.simp")
set(module_registry "${work}/simp-modules.tsv")
set(import_source "${work}/import.simp")
set(import_object "${work}/import.o")
set(import_executable "${work}/import executable")
execute_process(
    COMMAND "${CMAKE_COMMAND}" -E env "SIMP_MODULE_REGISTRY=${module_registry}"
            "CC=${CLANG}" "${COMPILER}" -c "${import_source}" -o "${import_object}"
    RESULT_VARIABLE result OUTPUT_VARIABLE stdout ERROR_VARIABLE stderr
)
if(NOT result EQUAL 0 OR NOT EXISTS "${import_object}")
    message(FATAL_ERROR "Compile-only with a source module failed:\n${stderr}")
endif()
execute_process(
    COMMAND "${CMAKE_COMMAND}" -E env "CC=${CLANG}" "${COMPILER}" "${import_object}"
            -o "${import_executable}"
    RESULT_VARIABLE result OUTPUT_VARIABLE stdout ERROR_VARIABLE stderr
)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "Linking the imported-module object failed:\n${stderr}")
endif()
execute_process(
    COMMAND "${import_executable}"
    RESULT_VARIABLE result OUTPUT_VARIABLE stdout ERROR_VARIABLE stderr
)
if(NOT result EQUAL 0 OR NOT stdout STREQUAL "42\n")
    message(FATAL_ERROR "Imported-module object result was '${stdout}': ${stderr}")
endif()
endif()

if(CASE MATCHES "^(all|invalid_.*)$")
set(combined_object "${work}/combined.o")
set(library_directory "${work}/libraries")
if(CASE MATCHES "^(invalid_compile_object|invalid_emit_object)$")
    execute_process(COMMAND "${COMPILER}" -c "${helper}" "${main}" -o "${combined_object}"
        RESULT_VARIABLE result ERROR_VARIABLE stderr)
    if(NOT result EQUAL 0)
        message(FATAL_ERROR "Could not prepare invalid-input object: ${stderr}")
    endif()
endif()
if(CASE STREQUAL "all" OR CASE STREQUAL "invalid_start")
execute_process(
    COMMAND "${COMPILER}" "${main}" "${work}/second-start.simp" -o "${work}/bad"
    RESULT_VARIABLE result OUTPUT_VARIABLE stdout ERROR_VARIABLE stderr
)
if(result EQUAL 0 OR NOT stderr MATCHES "more than one top-level 'start' block")
    message(FATAL_ERROR "Multiple start blocks were not rejected:\n${stderr}")
endif()
endif()

if(CASE STREQUAL "all" OR CASE STREQUAL "invalid_missing_start")
execute_process(
    COMMAND "${COMPILER}" "${work}/no-start.simp" -o "${work}/missing-entry"
    RESULT_VARIABLE result OUTPUT_VARIABLE stdout ERROR_VARIABLE stderr
)
if(result EQUAL 0 OR NOT stderr MATCHES "programs must contain exactly one top-level 'start' block")
    message(FATAL_ERROR "A missing start block was not rejected:\n${stderr}")
endif()
endif()
if(CASE STREQUAL "all" OR CASE STREQUAL "invalid_duplicate_symbol")
execute_process(
    COMMAND "${COMPILER}" "${work}/duplicate-a.simp" "${work}/duplicate-b.simp"
            -o "${work}/duplicate-symbols"
    RESULT_VARIABLE result OUTPUT_VARIABLE stdout ERROR_VARIABLE stderr
)
if(result EQUAL 0 OR NOT stderr MATCHES "duplicate class 'Duplicate'")
    message(FATAL_ERROR "A duplicate class across inputs was not diagnosed:\n${stderr}")
endif()
endif()

if(CASE STREQUAL "all" OR CASE STREQUAL "invalid_overwrite")
execute_process(
    COMMAND "${COMPILER}" "${helper}" "${main}" -o "${main}"
    RESULT_VARIABLE result OUTPUT_VARIABLE stdout ERROR_VARIABLE stderr
)
if(result EQUAL 0 OR NOT stderr MATCHES "must not overwrite an input file")
    message(FATAL_ERROR "An output/input path collision was not rejected:\n${stderr}")
endif()
endif()
if(CASE STREQUAL "all" OR CASE STREQUAL "invalid_compile_object")
execute_process(
    COMMAND "${COMPILER}" -c "${combined_object}" -o "${work}/bad.o"
    RESULT_VARIABLE result OUTPUT_VARIABLE stdout ERROR_VARIABLE stderr
)
if(result EQUAL 0 OR NOT stderr MATCHES "accepts source inputs only")
    message(FATAL_ERROR "-c accepted an object input:\n${stderr}")
endif()
endif()
if(CASE STREQUAL "all" OR CASE STREQUAL "invalid_emit_object")
execute_process(
    COMMAND "${COMPILER}" "${combined_object}" --emit-llvm "${work}/bad.ll"
            -o "${work}/bad-executable"
    RESULT_VARIABLE result OUTPUT_VARIABLE stdout ERROR_VARIABLE stderr
)
if(result EQUAL 0 OR NOT stderr MATCHES "--emit-llvm requires")
    message(FATAL_ERROR "--emit-llvm accepted object-only inputs:\n${stderr}")
endif()
endif()
if(CASE STREQUAL "all" OR CASE STREQUAL "invalid_compile_library")
execute_process(
    COMMAND "${COMPILER}" -c "${helper}" -L "${library_directory}" -o "${work}/bad.o"
    RESULT_VARIABLE result OUTPUT_VARIABLE stdout ERROR_VARIABLE stderr
)
if(result EQUAL 0 OR NOT stderr MATCHES "cannot be used with -c")
    message(FATAL_ERROR "-c accepted library link flags:\n${stderr}")
endif()
endif()
if(CASE STREQUAL "all" OR CASE STREQUAL "invalid_check_library")
execute_process(
    COMMAND "${COMPILER}" --check-only "${helper}" -l multi_test
    RESULT_VARIABLE result OUTPUT_VARIABLE stdout ERROR_VARIABLE stderr
)
if(result EQUAL 0 OR NOT stderr MATCHES "cannot be used with --check-only")
    message(FATAL_ERROR "--check-only accepted library link flags:\n${stderr}")
endif()
endif()
if(CASE STREQUAL "all" OR CASE STREQUAL "invalid_unsupported")
execute_process(
    COMMAND "${COMPILER}" "${work}/unknown.txt" -o "${work}/bad"
    RESULT_VARIABLE result OUTPUT_VARIABLE stdout ERROR_VARIABLE stderr
)
if(result EQUAL 0 OR NOT stderr MATCHES "unsupported input file type")
    message(FATAL_ERROR "An unsupported input type was not rejected:\n${stderr}")
endif()
endif()
endif()
if(NOT CASE MATCHES "^(all|combined|object|mixed|library|module|invalid_(start|missing_start|duplicate_symbol|overwrite|compile_object|emit_object|compile_library|check_library|unsupported))$")
    message(FATAL_ERROR "Unknown multi-input case: ${CASE}")
endif()
