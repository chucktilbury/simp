set(CASE_NAME cwhip_rejects_explicit_virtual_base_initialization)
set(CASE_FIXTURE negative_virtual_base_explicit_super.cw)
set(CASE_EXPECTED_DIAGNOSTIC [==[virtual base constructors must use super virtual]==])
