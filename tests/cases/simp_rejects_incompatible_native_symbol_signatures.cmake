set(CASE_NAME simp_rejects_incompatible_native_symbol_signatures)
set(CASE_FIXTURE negative_extern_symbol_signature_conflict.simp)
set(CASE_EXPECTED_DIAGNOSTIC [==[is reused with an incompatible method signature]==])
