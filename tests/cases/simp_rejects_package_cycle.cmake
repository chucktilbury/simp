set(CASE_NAME simp_rejects_package_cycle)
set(CASE_FIXTURE negative_package_cycle.simp)
set(CASE_PACKAGE_FIXTURE "${CMAKE_CURRENT_LIST_DIR}/../functional/modules/packages")
set(CASE_EXPECTED_DIAGNOSTIC "cyclic package dependency involving 'cycle_a'")
