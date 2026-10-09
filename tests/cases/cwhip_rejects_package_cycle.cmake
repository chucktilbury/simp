set(CASE_NAME cwhip_rejects_package_cycle)
set(CASE_FIXTURE negative_package_cycle.cw)
set(CASE_MODULE_ROOT packages)
set(CASE_EXPECTED_DIAGNOSTIC "cyclic package dependency involving 'cycle_a'")
