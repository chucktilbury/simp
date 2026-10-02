# Tests

This directory contains C++ unit/structural tests, CTest case definitions, and
functional Simple programs. Build the repository with CMake, then run the full
suite from the repository root:

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

## Adding an integration test

Integration tests are discovered from individual
`cases/*.cmake` files. Add a `positive_*.simp` or `negative_*.simp` source
under `functional/positive/` or `functional/negative/`, then add a case file setting
`CASE_NAME` (the CTest name) and `CASE_FIXTURE` (the source basename).

For a successful compile-and-run case, set `CASE_EXPECTED_OUTPUT_FILE` to a
neighboring `.stdout` file containing the exact output, including its final
newline. For a rejected program, set `CASE_EXPECTED_DIAGNOSTIC` to the expected
diagnostic regular expression. Optional flags include
`CASE_REQUIRE_GC_ROOTS`, `CASE_REQUIRE_VIRTUAL_DISPATCH`,
`CASE_EXPECT_WARNING`, `CASE_EXPECT_RUNTIME_FAILURE`,
`CASE_EXPECT_RUNTIME_DIAGNOSTIC`, and `CASE_MODULE_REGISTRY`.

Module tests set `CASE_MODULE_ROOT` to a fixture directory under
`functional/modules/`, copied into a private module root. Set
`CASE_MODULE_ROOT_SOURCE` to choose how the root is selected: `cli` (the
default, via `-M`), `env`, `default`, `stdlib`, or the deprecated
`package-path`/`package-env`. `CASE_MODULE_ROOT_MISSING` leaves the selected
root absent. `CASE_MODULE_DECOY` copies a fixture into lower-priority roots to
verify that root precedence is respected.

Other useful options are `CASE_ARGUMENTS`, `CASE_NO_RUN`,
`CASE_NO_SOURCE`, `CASE_EXPECT_COMPILE_OUTPUT`, and
`CASE_REJECT_COMPILE_OUTPUT`. Output expectations may use `<MODULE_ROOT>`,
`<PROJECT_DIR>`, and `<WORK_DIR>` placeholders. Every case runs in its own
work directory with the `SIMP_*` path variables cleared. `CASE_DEBUG_INFO`
adds a `-g` case that checks generated DWARF data and, when GDB or LLDB is
installed, exercises a breakpoint and local-variable inspection. See a
neighboring case file and its runner for details. Reconfigure after adding a
case so CMake discovers it; no central test list needs editing.

## Unit and CLI tests

Parser and semantic fixtures consumed by `simp_tests` use a
`<fixture>.simp.json` file beside the `.simp` file with `friendly_name`,
`expected_diagnostic` (empty for accepted input), and an `enabled` boolean.
Focused C++ structural assertions belong in a `test_*_cases.cpp` group using
`TestGroupRegistration` from `test_cases.hpp`; CMake discovers these groups.
Command-line integration fixtures live under `functional/cli/`; the shared
runner accepts a `CASE` selector so each scenario has an independent CTest
result.
