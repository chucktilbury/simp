project(simp)

find_package(FLEX REQUIRED)
find_package(BISON REQUIRED)

BISON_TARGET(PARSER
    parser.y
    ${CMAKE_CURRENT_BINARY_DIR}/parser.c
    COMPILE_FLAGS "--name-prefix=simp_ --report=lookahead -tvdo"
)

FLEX_TARGET(SCANNER
    scanner.l
    ${CMAKE_CURRENT_BINARY_DIR}/scanner.c
    COMPILE_FLAGS "-i --prefix=simp_"
)

ADD_FLEX_BISON_DEPENDENCY(SCANNER PARSER)

add_executable(${PROJECT_NAME}
    ${BISON_PARSER_OUTPUTS}
    ${FLEX_SCANNER_OUTPUTS}
    main.c
    memory.c
)

target_link_libraries(${PROJECT_NAME}
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
    $<$<CONFIG:DEBUG>:-DENABLE_AST_DUMP>
    $<$<CONFIG:DEBUG>:-DENABLE_PARSER_TRACE>
    $<$<CONFIG:DEBUG>:-g>
    $<$<CONFIG:RELEASE>:-Ofast>
    $<$<CONFIG:PROFILE>:-pg -O0>
)
