set(CASE_NAME cwhip_rejects_duplicate_out_of_line_method_definition)
set(CASE_FIXTURE negative_extern_duplicate_name.cw)
set(CASE_EXPECTED_DIAGNOSTIC [==[duplicate out-of-line definition for method 'Native.absolute']==])
