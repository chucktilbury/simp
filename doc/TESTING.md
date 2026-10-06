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
generated programs. Leak checking of the compiler alone can be enabled manually.

`SIMP_STAGE_PREFIX` defaults to the source tree for the traditional `bin/simp`
layout. Give instrumented builds a distinct prefix inside their build directory
to avoid overwriting another configuration's compiler and runtime.

The networking integration case exercises empty native strings; UBSan caught
and now guards the zero-length `memchr` call on their null backing storage.

## Repository examples

`ctest --test-dir build -L examples --output-on-failure -j4` compiles every
`examples/**/*.simp`. Each program runs in a temporary project with a private
home directory, checked stdout, checked exit status, and checked stderr.
`scanner.simp` receives the checked-in `input_test.txt` in its working directory
and on stdin. The formerly empty module example now exports `Example.Answer`;
a small importing driver compiles and runs it, checking its answer.

`tests/examples.json` contains exact expectations; configuration fails if any
example has no expectation or an expectation names a deleted example.
`err.simp` handles a missing-file error and exits successfully; `test.simp`
demonstrates a buffer-bounds runtime error and `unhandled_exception.simp`
demonstrates an uncaught exception. The latter two must abort with the intended
diagnostic, not a sanitizer failure. Broken string-call syntax in
`virtual_base.simp` and the mismatched inline capture in `test.simp` have been
updated to the current language.
