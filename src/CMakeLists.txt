project(simp)

add_executable(${PROJECT_NAME}
    parser.c
    scanner.c
    filebuf.c
    main.c
    memory.c
    errors.c
    ptrlst.c
    cmdline.c
    strptr.c
    tokens.c
    expressions.c
    module.c
    reference.c
    function.c
    class.c
    #ast.c
    #dump.c
    #import.c
)

add_custom_target(
    keywords.h ALL
    python3 ${PROJECT_SOURCE_DIR}/sort.py ${PROJECT_SOURCE_DIR}/keywords.txt
    DEPENDS ${PROJECT_SOURCE_DIR}/keywords.txt
)

target_link_libraries(${PROJECT_NAME}
    gc
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        ${CMAKE_CURRENT_BINARY_DIR}
        ${PROJECT_SOURCE_DIR}
)

target_compile_options(${PROJECT_NAME} PRIVATE
    -Wall
    -Wextra
    -Wno-unused-parameter
    -Wno-sign-compare
    -Wno-unused-variable
    -Wno-parentheses-equality
    -DENABLE_GC
    #$<$<CONFIG:DEBUG>:-DENABLE_AST_DUMP>
    $<$<CONFIG:DEBUG>:-DENABLE_PARSER_TRACE>
    #$<$<CONFIG:DEBUG>:-DENABLE_SCANNER_TRACE>
    $<$<CONFIG:DEBUG>:-DENABLE_TOKENS_TRACE>
    $<$<CONFIG:DEBUG>:-DENABLE_DBG_MSG>
    $<$<CONFIG:DEBUG>:-g>
    $<$<CONFIG:RELEASE>:-Ofast>
    $<$<CONFIG:PROFILE>:-pg -O0>
)
