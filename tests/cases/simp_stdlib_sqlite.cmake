set(CASE_NAME simp_stdlib_sqlite)
set(CASE_FIXTURE positive_stdlib_sqlite.simp)
file(STRINGS "${CMAKE_CURRENT_LIST_DIR}/../functional/positive/positive_stdlib_sqlite.simp"
    sqlite_assertions REGEX "^[ \t]*print\\(")
list(LENGTH sqlite_assertions sqlite_assertion_count)
string(REPEAT "true" ${sqlite_assertion_count} CASE_EXPECTED_OUTPUT)
set(CASE_MODULE_ROOT_SOURCE stdlib)
set(CASE_MODULE_ROOT "${SIMP_STAGE_PREFIX}/${CMAKE_INSTALL_DATADIR}/simp/modules")
