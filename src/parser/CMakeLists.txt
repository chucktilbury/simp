project(parser)

find_package(FLEX 2.6 REQUIRED)
find_package(BISON 3.8 REQUIRED)

BISON_TARGET(PARSER
    parser.y
    ${CMAKE_CURRENT_BINARY_DIR}/parser.c
    DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/parser.h
    COMPILE_FLAGS ""
)

FLEX_TARGET(SCANNER
    scanner.l
    ${CMAKE_CURRENT_BINARY_DIR}/scanner.c
    DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/scanner.h
    COMPILE_FLAGS
)

ADD_FLEX_BISON_DEPENDENCY(SCANNER PARSER)

file(GLOB_RECURSE ${PROJECT_NAME}_files
    ${PROJECT_SOURCE_DIR}/*.c
)

add_library(${PROJECT_NAME} STATIC
    ${BISON_PARSER_OUTPUTS}
    ${FLEX_SCANNER_OUTPUTS}
    ${${PROJECT_NAME}_files}
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        ${CMAKE_CURRENT_BINARY_DIR}
        ${PROJECT_SOURCE_DIR}
        ${PROJECT_SOURCE_DIR}/../common
        ${PROJECT_SOURCE_DIR}/../lists
        ${PROJECT_SOURCE_DIR}/../render
        ${PROJECT_SOURCE_DIR}/../parser
        ${PROJECT_SOURCE_DIR}/../pgen
        ${PROJECT_SOURCE_DIR}/../gc
)

target_compile_options(${PROJECT_NAME} PRIVATE
    -Wall
    -Wextra
    -Wno-unused-parameter
    -Wno-sign-compare
    -Wno-unused-variable
    -Wno-parentheses-equality
    -DUSE_TRACE
    #-DUSE_GC
    $<$<CONFIG:DEBUG>:-DENABLE_AST_DUMP>
    #$<$<CONFIG:DEBUG>:-DENABLE_PARSER_TRACE>
    $<$<CONFIG:DEBUG>:-g>
    $<$<CONFIG:RELEASE>:-Ofast>
    $<$<CONFIG:PROFILE>:-pg -O0>
)
