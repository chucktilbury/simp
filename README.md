# Simple compiler

Simple is a statically typed language with classes, inheritance, managed
strings and collections, exceptions, and native bindings. This repository
contains its C++ compiler, runtime, standard-library packages, and tests. The
user documentation is indexed in [`doc/README.md`](doc/README.md); the
[language reference](doc/LANGUAGE-REFERENCE.md) and [grammar](doc/GRAMMAR.md)
describe the implemented language.

## Build and test

Configure and build the compiler and runtime with CMake, then run the test
suite with CTest:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j4
ctest --test-dir build --output-on-failure
```

The build places the compiler at `bin/simp` and stages the runtime, String
builtin, and standard modules beside it in the repository's `lib/`, `include/`,
and `share/` directories. To compile and run a program:

```sh
./bin/simp path/to/program.simp -o build/program
./build/program
```

See [`doc/simp.1`](doc/simp.1) for compiler options and
[`doc/README.md`](doc/README.md) for all language and library documentation.
The compiler's [test guide](tests/README.md) explains the functional fixtures
and how to add cases; runnable language examples are in
[`doc/EXAMPLES.md`](doc/EXAMPLES.md).

## Install

```sh
cmake -S . -B build -DCMAKE_INSTALL_PREFIX=/usr/local
cmake --build build
cmake --install build
```

The installed compiler locates its runtime, builtin, standard modules, and
documentation relative to its installation prefix. CMake install rules honor
`DESTDIR` for staged installations. See
[`doc/INSTALLATION.md`](doc/INSTALLATION.md) for the complete installed layout,
resource overrides, package search order, and
[project version policies](doc/INSTALLATION.md#project-module-version-selection).
The `simpkg` package manager provides direct dependencies, a reproducible
lockfile, and complete dependency installation with explicit network consent:

```sh
simpkg init
simpkg add OWNER/REPO --yes
simp src/app.simp
```

No environment activation is needed. The manifest schema is in
[`doc/PACKAGES.md`](doc/PACKAGES.md), and command behavior is documented in
[`doc/INSTALLATION.md`](doc/INSTALLATION.md#project-package-manager-simpkg).

## Repository layout

| Path | Contents |
| --- | --- |
| `src/` | Compiler front end, LLVM IR generator, command-line driver, and runtime |
| `include/` | Runtime C headers and compiler headers |
| `builtin/` | Built-in `String` class source |
| `stdlib/` | Versioned standard-library packages |
| `tests/` | Compiler tests and Simple source fixtures |
| `doc/` | User documentation |
| `build/`, `bin/`, `lib/`, `share/` | Local CMake build outputs |

The standard library's package layout and contribution process are described
in [`stdlib/README.md`](stdlib/README.md); its public APIs are in
[`doc/STDLIB.md`](doc/STDLIB.md).

## Implemented language highlights

- `int` is signed 64-bit; integer literals support decimal and `0x` hexadecimal
  forms with checked ranges. The string type keyword is `strg`; double-quoted
  strings support `\e`. `format("value {}", value)` returns `strg`, and
  `print("{value:08X}", value=42)` formats directly. Literal templates support
  positional/named fields, alignment, width, decimal/hex integers, and
  ASCII-only `c`; string-literal calls are no longer supported.
- Scalar `int`, `unsigned`, and `float` variables and fields support
  `+=`, `-=`, `*=`, `/=`, and the applicable `%=` compound assignments.
- Anonymous class enums declare immutable signed 64-bit `int` constants
  accessible as `Class.NAME` or `object.NAME`, without creating a new type or
  per-instance storage.
- Base constructors use dotless `super Base(args)` syntax; virtual bases use
  `super virtual Base(args)` or `virtual super Base(args)`.
- Collection type keywords are `list` and `dict`; `array` and `map` are
  ordinary identifiers. The `system` standard package includes `Sys.Glob`.

These are brief feature summaries, not a substitute for the references linked
above.
