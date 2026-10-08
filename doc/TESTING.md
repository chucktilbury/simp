# Testing infrastructure

The root CMake build registers all tests with CTest. Python 3.11+, Clang,
a C11/C++17 compiler, CMake 3.16+, and POSIX threads are required.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j4
ctest --test-dir build --output-on-failure -j4
```

## Sanitizers

Use Clang for both native languages so its sanitizer runtime also matches the
Clang driver used for generated programs:

```sh
cmake -S . -B build-asan -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ \
  '-DSIMP_SANITIZE=address;undefined' \
  -DSIMP_STAGE_PREFIX="$PWD/build-asan/stage"
cmake --build build-asan -j4
ctest --test-dir build-asan --output-on-failure -j4
```

`SIMP_SANITIZE` defaults to `OFF`. It instruments the compiler, C runtime,
and native tests. That compiler automatically forwards sanitizer flags when
compiling IR, inline C, and linking executables, including compile-only objects.
Externally supplied native libraries are not automatically instrumented.
ASan cannot instrument every operation in hand-written LLVM IR; runtime and
inline C accesses are instrumented.

CTest sets `ASAN_OPTIONS=detect_leaks=0:halt_on_error=1` and
`UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1`. Leak detection is disabled
because the managed heap intentionally retains live objects until process exit
(including exceptional exits); address and UB checks remain fatal, and no tests
are excluded. Outside CTest, use those environment variables when running
generated programs. Frontend-only unit tests retain leak detection, as does
the fuzz harness; leak checking of the compiler CLI can also be enabled manually.
Integration drivers reject sanitizer output even in expected abort/exit cases,
so a sanitizer crash cannot satisfy an intentional-failure expectation.

`SIMP_STAGE_PREFIX` defaults to the source tree for the traditional `bin/simp`
layout. Give instrumented builds a distinct prefix inside their build directory
to avoid overwriting another configuration's compiler and runtime.

The networking integration case exercises empty native strings; UBSan caught
and now guards the zero-length `memchr` call on their null backing storage.

## Real GTK 4 interface

The optional suite is enabled explicitly, never as a silently skipped display
test:

```sh
cmake -S . -B build-gtk -DSIMP_GTK=ON -DSIMP_GTK_SOURCEVIEW=ON -DSIMP_GTK_TESTS=ON \
  -DCMAKE_C_FLAGS=-Werror=deprecated-declarations \
  -DSIMP_STAGE_PREFIX="$PWD/build-gtk/stage"
cmake --build build-gtk -j4
cmake --build build-gtk --target tweed -j4
ctest --test-dir build-gtk -L gtk --output-on-failure
```

This explicit test configuration requires `pkg-config`, GTK 4.10+ and GtkSourceView 5 development
files, Xvfb, and `dbus-daemon` (`pkg-config libgtk-4-dev
libgtksourceview-5-dev xvfb dbus-daemon` on Debian/Ubuntu).
A local Xvfb executable can
be supplied via `SIMP_XVFB_EXECUTABLE`; it must have its normal shared-library
dependencies available. `gtk_support.py` starts a fresh Xvfb using `-displayfd` and a private session
bus, checks both for readiness, uses private HOME/configuration, and selects
GTK's Cairo renderer with GL disabled. It terminates those exact owned processes
even when a consumer fails. There is no dependence on the user's running desktop
or application IDs on their bus. Startup failure and missing display are failures.

Compiled Simple cases cover all public widgets/properties, typed
clicked/changed/toggled/close signals, copied text under GC, boolean close
responses, nested emissions, retained receivers, disconnect/disposal/shutdown
inside handlers, recursive parent/child disposal, detached children, aliases,
native window destruction, native weak-ref finalization, cancellation, worker
progress and GUI posts, and misuse/exception aborts. Compile failures check
callback types and package-private access. Two processes on one isolated bus
verify actual single-instance activation forwarding. The same test verifies
compile-only link sidecars, installs scheduler/signal consumers and a public
widget consumer using `simpkg init`/`install` and its lock, and checks absence of
GTK linkage in non-import programs and the compiler. Fixtures live in
`tests/functional/cli/gtk/`, not the public package.
`simp_callbacks` separately checks one-shot transport roots, accept on another
registered thread, cancellation, lock reentry and unregistered-thread rejection,
as well as the pre-existing strict foreign-owner invocation failures.

The editor example test exercises the Tweed language definition, text-buffer
editing/search, undo/redo, document/tab behavior and file operations. The
shortcut test covers defaults, user mappings and conflicts. Run the complete
configured suite with `ctest --test-dir build-gtk --output-on-failure`.
`simp_gtk_bindings` checks actual chooser accept/cancel and overwrite responses,
immediate cancellation/parent disposal/shutdown for all four dialog operations,
prompt GC release of callback receivers, and native weak-ref finalization after
late cancelled completions (including after application shutdown).
`simp_editor_dialogs` checks multi-select acceptance/cancellation and Save/Save As
overwrite decisions without losing tabs or changing files on cancellation.
`simp_project_explorer` checks folder acceptance/cancellation and both immediate
and mapped parent/shutdown teardown, alongside the existing tree regressions.
Only test fixtures use deprecated chooser APIs to drive GTK's real fallback UI;
the package build remains checked with deprecated declarations as errors.
`simp_tweed_binary` runs the actual `tweed` target output with the same bounded
editor fixture, private Xvfb and session bus; its `simp_tweed_build` CTest
fixture builds the target first. GUI options default to installed development
dependencies on fresh configurations; the explicit options above ensure this
test configuration fails rather than becoming compiler-only.
The build fixture also checks nanosecond output timestamps on a second
invocation so an unchanged target cannot silently recompile or relink.

For sanitizer coverage, add `-DSIMP_GTK=ON` and `SIMP_XVFB_EXECUTABLE` if needed
to the sanitizer build above, then run the same selector with `-j1` to avoid
resource contention in GUI timing checks. Address/UB checks
remain fatal. The existing generated-program leak policy (`detect_leaks=0`)
also avoids reporting GTK/font/display process-global caches at exit; it is
not a suppression of GTK memory access failures. No GTK tests are skipped,
and external uninstrumented GTK libraries are not claimed to be instrumented.

## Repository examples

`ctest --test-dir build -L examples --output-on-failure -j4` compiles every
configured `examples/**/*.simp`. Each program runs in a temporary project with a private
home directory, checked stdout, checked exit status, and checked stderr.
`scanner.simp` receives the checked-in `input_test.txt` in its working directory
and on stdin. The formerly empty module example now exports `Example.Answer`;
a small importing driver compiles and runs it, checking its answer.

`tests/examples.json` contains exact expectations; configuration fails if any
example has no expectation or an expectation names a deleted example.
`gtk.simp` explicitly requires optional GTK: it is registered only with
`SIMP_GTK=ON`, and runs with `--test` on the headless fixture. Without that
argument the example is interactive. Compiler-only configurations need no GTK;
their inventory still validates optional examples.
`err.simp` handles a missing-file error and exits successfully; `test.simp`
demonstrates a buffer-bounds runtime error and `unhandled_exception.simp`
demonstrates an uncaught exception. The latter two must abort with the intended
diagnostic, not a sanitizer failure. Broken string-call syntax in
`virtual_base.simp` and the mismatched inline capture in `test.simp` have been
updated to the current language.

## Documentation programs

`ctest --test-dir build -L documentation --output-on-failure` extracts and
executes complete `simp`/`simple` fenced blocks from every `doc/*.md`.
A block containing `start {` must include exactly one Simple comment:

```text
// test: {"stdout": "exact expected output\n"}
```

This is JSON: escape newlines and quotes, and include no implicit line breaks
(`print` adds none). The marker is also the documented, machine-checked output.
`<WORK_DIR>` in stdout expands to the isolated program working directory.
Optional `fixture` names a checked-in project under `tests/doc_fixtures/`
for examples that demonstrate external packages. Optional `stdin`, `exit`,
and `diagnostic` use the same checks as repository examples.
`"requires": "gtk"` explicitly identifies an optional GTK program. The runner
validates its marker in all builds, reports it as not configured with
`SIMP_GTK=OFF`, and compiles/runs it on the headless fixture with `SIMP_GTK=ON`.
Configured GTK programs are never silently skipped; missing fixtures/toolkit
dependencies fail. This includes the complete sample in `GTK.md`.
Explicit `// Fragment: reason` comments identify non-standalone snippets;
they cannot carry test markers. The extractor refuses unmarked complete
programs rather than silently skipping new ones. Design-note fragments without
entry points are not executable tests. The inline C example's numeric output
comment has been corrected from `3.0` to the actual default rendering `3`.

## Continuous integration

`.github/workflows/tests.yml` runs the full suite on `ubuntu-latest` for both
default and ASan+UBSan builds on pushes and pull requests. Both use Clang and
build-local staging, with no downloaded Simple packages or network tests.
Package-manager integration tests use local Git repositories.

On Ubuntu, the same prerequisites can be installed with:

```sh
sudo apt-get update
sudo apt-get install -y build-essential cmake clang llvm python3 git gdb
```

LLVM supplies `opt` and `llvm-dwarfdump` for IR and debug-info checks; the
compiler itself does not link the LLVM C++ API. Python must be at least 3.11
(as supplied by current Ubuntu). `scripts/setup` is an optional developer
shell helper, not a dependency installer. Mirror CI with the commands above,
`-DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++`, and a private
`HOME`/`XDG_CONFIG_HOME` when invoking CTest.

## Coverage

`SIMP_COVERAGE=ON` enables Clang's source-based coverage for native C/C++,
including the runtime linked into generated executables and inline C shims.
It defaults to `OFF` and requires matching Clang, `llvm-profdata`, and
`llvm-cov` versions (Ubuntu's `clang llvm` packages). No gcovr/lcov dependency
is needed.

```sh
cmake -S . -B build-coverage -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ \
  -DSIMP_COVERAGE=ON -DSIMP_STAGE_PREFIX="$PWD/build-coverage/stage"
cmake --build build-coverage -j4
cmake --build build-coverage --target coverage
```

The `coverage` target clears only its raw profiles, runs the full CTest suite,
then writes `coverage/summary.txt`, `coverage/coverage.json`, and
`coverage/html/index.html` below the build directory. CTest gives every
process/module its own profile filename so concurrent compiler/program runs
do not overwrite one another. `scripts/coverage.py` also accepts explicit
`--build`, `--source`, and repeated `--binary` arguments to report existing
profiles without rerunning tests. Reports combine compiler, native unit tests,
and generated integration executables, restricted to `src/` C/C++ files.
Hand-generated Simple LLVM IR has no C/C++ coverage source mapping.
Processes that abort cannot flush all counters; their already-completed
compiler invocations are still covered.

## Lexer/parser fuzzing

`SIMP_FUZZ=ON` adds `simp_fuzz_frontend`, using Clang's libFuzzer with
ASan+UBSan. It requires `SIMP_SANITIZE=address;undefined`; both options are
off by default. The front end is compiled with coverage-guided instrumentation,
not just the harness. The harness lexes arbitrary bytes and parses programs or
declaration-only modules; only `DiagnosticError` is treated as expected.
It never expands includes, loads packages, compiles inline C, or runs generated
code. Semantic analysis and the runtime are outside this fuzz target.

```sh
cmake -S . -B build-fuzz -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ \
  '-DSIMP_SANITIZE=address;undefined' -DSIMP_FUZZ=ON \
  -DSIMP_STAGE_PREFIX="$PWD/build-fuzz/stage"
cmake --build build-fuzz -j4
ctest --test-dir build-fuzz -L fuzz --output-on-failure
python3 scripts/fuzz.py --fuzzer build-fuzz/stage/bin/simp_fuzz_frontend \
  --fixtures tests/functional --work build-fuzz/fuzz --seconds 180
```

The smoke test runs at most 2,000 iterations or ten seconds. The longer script
defaults to three minutes, a 16 KiB mutation limit, a five-second per-input
timeout, and a 2 GiB RSS limit. All functional `.simp` fixtures seed a writable,
deduplicated corpus under the selected build-local work directory. Mutations
persist there for subsequent campaigns; crash/hang artifacts stay in
`artifacts/`. Reproduce an artifact by passing its path directly to the fuzzer.
Leak detection stays enabled here: unlike generated programs, the parser's AST
and tokens should release all memory after each input.

A deep-parenthesis stress seed exposed parser stack exhaustion. The parser
now diagnoses excessive recursive nesting instead of overflowing; regressions
cover parentheses, unary operators, nested blocks/namespaces, and nested calls.
The campaign also includes these generated stress seeds.
An additional long additive chain overflowed semantic normalization despite
not nesting the parser. Expression tree depth is now validated during
construction, including binary/member chains and inline capture paths, so the
compiler diagnoses it before recursive analysis or destruction can overflow.
