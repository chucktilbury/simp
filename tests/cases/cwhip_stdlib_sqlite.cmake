set(CASE_NAME cwhip_stdlib_sqlite)
set(CASE_FIXTURE positive_stdlib_sqlite.cw)
file(STRINGS "${CMAKE_CURRENT_LIST_DIR}/../functional/positive/positive_stdlib_sqlite.cw"
    sqlite_assertions REGEX "^[ \t]*print\\(")
list(LENGTH sqlite_assertions sqlite_assertion_count)
string(REPEAT "true" ${sqlite_assertion_count} CASE_EXPECTED_OUTPUT)
set(CASE_MODULE_ROOT_SOURCE stdlib)
set(CASE_MODULE_ROOT "${CWHIP_STAGE_PREFIX}/${CMAKE_INSTALL_DATADIR}/cwhip/modules")
