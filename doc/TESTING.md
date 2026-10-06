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
