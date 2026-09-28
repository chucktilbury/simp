# Simple compiler prototype

This repository contains the first runnable C++ front end for Simple. It is a
small, deliberately incomplete prototype, not an implementation of the full
language in `SIMPLE-LANGUAGE-NOTES.md`.

## Build and run

The root CMake project integrates the header, compiler, and test subprojects:

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
./bin/simp tests/functional/positive_integer_output.simp
./bin/positive_integer_output
# Prints: 42
./bin/simp tests/functional/positive_integer_output.simp \
  --emit-llvm build/positive_integer_output.ll -o bin/positive_integer_output
```

The root build writes executables to project-root `bin/` and static, shared, or
module libraries to project-root `lib/` (the front-end archive is
`lib/libsimp_frontend.a` on Linux).

The repository uses one in-tree build directory: `build/`. `include`, `src`,
and `tests` each have their own `CMakeLists.txt` and are integrated by the root
project; standalone configuration is optional. To configure a component
without creating another build directory in the repository, use a temporary
directory outside it, for example:

```sh
cmake -S src -B /tmp/simp-compiler-build
cmake --build /tmp/simp-compiler-build
```

Standalone component executables and libraries stay under that external build
directory. `tests/functional` contains source fixtures, not a separate
buildable component. The root-integrated build puts both executables and its
front-end archive in the project-root `bin/` and `lib/`, respectively.

Useful options are `--verbose` (`-v`), `--trace-parser`, `--dump-ast`,
`--dump-symbols`, and `--check-only` (run parsing and semantic checks without
code generation). LLVM IR is compiled to a native executable by the installed
Clang driver; `--emit-llvm FILE` additionally saves the generated IR.
Executables default to `bin/<input-basename>`; `-o FILE` selects another path.
The compiler reads one source file and reports source-located lexer, parser,
and semantic errors.

## Implemented subset

- Exactly one top-level `start { ... }` block is required. Duplicate or missing
  blocks are errors; other top-level forms are rejected.
- Reserved keywords are case-insensitive: `start`, `int`, `string`, `if`,
  `else`, `while`, and `print`. Every capitalization is reserved.
- `int` and `string` declarations (with optional initializer) and identifier
  assignment use semicolon-terminated statements.
- Expressions include integer and string literals, identifiers, parentheses,
  unary `+`, `-`, `!`, arithmetic `+ - * / %`, and comparisons `== != < <= > >=`.
- `if (condition) { ... }`, unconditional `else { ... }`, and
  `while (condition) { ... }` are supported. An `else (condition)` form is
  explicitly rejected.
- `print(expr, ...);` accepts zero or more expressions. Arguments are parsed
  into the AST; a double-quoted first argument uses basic `{}` formatting and
  must have one placeholder per following value. Single-quoted strings are
  literal and do not interpolate.
- `//` line comments and basic double-quoted escapes (`\\`, `\"`, `\n`, `\r`,
  `\t`) are accepted. Single-quoted strings have no escape processing.
- Semantic analysis resolves lexical local names, rejects use before
  initialization and undeclared identifiers, checks `int`/`string`
  initialization and assignment types, validates integer literal range, and
  requires integer conditions. Definite initialization across `if` branches
  and loops is conservative.

The parser is recursive descent and produces an AST that can be dumped with
`--dump-ast`. Lexer, parser, and CLI diagnostics include file, line, and column.
Parser tracing is available with `--trace-parser`; `--dump-symbols` displays
the declarations and initialization state seen by semantic analysis.

## Executable backend subset

The current backend compiles straight-line programs containing initialized or
uninitialized `int` declarations, integer assignments, integer arithmetic and
comparisons, and `print` with exactly one integer expression. The generated
LLVM IR is textual IR using opaque pointers, then the configured Clang
executable compiles and links it. The program entry returns zero.

The parser and semantic analyzer accept more syntax than the backend can
execute. In particular, `string` declarations/expressions, formatted or
multiple-value print, `if`, and `while` currently produce an explicit
backend-not-supported diagnostic instead of an executable. There is no
runtime bounds or division-by-zero handling; signed division follows LLVM
integer operation semantics.

## Deferred

LLVM 22.1.8, CMake 3.31.6, GCC 14.2, and Clang 22.1 were available when this
prototype was extended. The implementation emits textual LLVM IR and invokes
Clang; it does not link the LLVM C++ API or provide a configurable LLVM
optimization pipeline. Building the compiler requires Clang on `PATH`; the
current driver launches it through the host POSIX shell. Full language type
checking and name-resolution rules,
garbage collection (GC), modules and native libraries, inline C, GTK, package
manager, IDE, and debugger remain deferred. The full grammar, classes/OOP,
collections, imports/includes, and the remaining semantics in the design notes
are not implied to work.
