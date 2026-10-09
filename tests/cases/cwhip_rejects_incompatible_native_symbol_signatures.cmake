set(CASE_NAME cwhip_rejects_incompatible_native_symbol_signatures)
set(CASE_FIXTURE negative_extern_symbol_signature_conflict.cw)
set(CASE_EXPECTED_DIAGNOSTIC [==[is reused with an incompatible method signature]==])
