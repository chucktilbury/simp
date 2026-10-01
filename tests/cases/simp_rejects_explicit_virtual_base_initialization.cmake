set(CASE_NAME simp_rejects_explicit_virtual_base_initialization)
set(CASE_FIXTURE negative_virtual_base_explicit_super.simp)
set(CASE_EXPECTED_DIAGNOSTIC [==[virtual base constructors must use super virtual]==])
