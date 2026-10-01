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
./bin/simp tests/functional/positive/positive_integer_output.simp -o bin/positive_integer_output
./bin/positive_integer_output
# Prints: 42
./bin/simp tests/functional/positive/positive_integer_control_flow.simp -o bin/positive_integer_control_flow
./bin/positive_integer_control_flow
# Prints: 9
./bin/simp tests/functional/positive/positive_string_format.simp -o bin/positive_string_format
./bin/positive_string_format
# Prints café and a blank line, then "value: 42" and "sum 21 21".
./bin/simp tests/functional/positive/positive_class_counter.simp -o bin/positive_class_counter
./bin/positive_class_counter
# Prints: 42, then 42
./bin/simp tests/functional/positive/positive_gc_object_graph.simp -o bin/positive_gc_object_graph
./bin/positive_gc_object_graph
# Prints: 1, 64, and 77 after repeated collections.
./bin/simp tests/functional/positive/positive_multiple_inheritance.simp -o bin/positive_multiple_inheritance
./bin/positive_multiple_inheritance
# Prints: 7, 7, 10, 20, and 3; the two Root subobjects hold separate Node references.
./bin/simp tests/functional/positive/positive_secondary_bases.simp -o bin/positive_secondary_bases
./bin/positive_secondary_bases
# Exercises secondary-base construction, conversions, dispatch, GC tracing, and destruction.
./bin/simp tests/functional/positive/positive_integer_output.simp \
  --emit-llvm build/positive_integer_output.ll -o bin/positive_integer_output
```

The root build writes executables to project-root `bin/` and stages the
compiler's resources beside it in the same relative shape as an installation,
so `./bin/simp` runs straight from the source tree without any compiled-in
source or build paths:

| Resource | Staged (root build) | Installed (`GNUInstallDirs`) |
| --- | --- | --- |
| Compiler | `bin/simp` | `${CMAKE_INSTALL_BINDIR}/simp` |
| Runtime archive | `lib/simp/libsimp_runtime.a` | `${CMAKE_INSTALL_LIBDIR}/simp/libsimp_runtime.a` |
| Runtime C headers | `include/simp/Runtime*.h` | `${CMAKE_INSTALL_INCLUDEDIR}/simp/` |
| String prelude | `share/simp/prelude/String.simp` | `${CMAKE_INSTALL_DATADIR}/simp/prelude/` |
| Standard modules | `share/simp/modules/` | `${CMAKE_INSTALL_DATADIR}/simp/modules/` |
| Documentation | — | `${CMAKE_INSTALL_DOCDIR}`, `${CMAKE_INSTALL_MANDIR}/man1/simp.1` |

In the repository source tree, the String prelude lives in `prelude/String.simp`
and standard-library modules originate under `stdlib/`
(`stdlib/<name>/<version>/simp-package.toml`), while `include/` contains only
C runtime headers and C++ compiler headers. `stdlib/` is deliberately named
differently from `modules/` so that compiling a `.simp` file at the repository
root does not treat the standard library as the project module root
(`<project-root>/modules`). CMake stages `prelude/` into `share/simp/prelude/`
and `stdlib/` into `share/simp/modules/`.

The front-end archive used by the test executables is `lib/libsimp_frontend.a`.
The runtime lives in a `simp` subdirectory of the library directory so that a
future shared runtime can sit beside the archive without colliding with system
libraries; headers and data likewise use a `simp` subdirectory so they never
shadow other packages. `share/` and `lib/` in the source tree are build
artifacts (ignored by git).

### Installing

```sh
cmake -S . -B build -DCMAKE_INSTALL_PREFIX=/usr/local
cmake --build build
cmake --install build                       # or: DESTDIR=/tmp/stage cmake --install build
```

Install rules use `GNUInstallDirs` and honour `DESTDIR`. The install
directories must stay inside `CMAKE_INSTALL_PREFIX` (configuration fails
otherwise) because the compiler is relocatable: it finds the running
executable (`/proc/self/exe` on Linux, the `KERN_PROC_PATHNAME` sysctl on
FreeBSD/DragonFly/NetBSD, otherwise `argv[0]` or a `PATH` search), strips the
binary directory to obtain the prefix, and derives every resource from it. An
installed tree can therefore be moved as a unit. Resolution order for each
resource is its specific variable (`SIMP_RUNTIME_DIR`, `SIMP_INCLUDE_DIR`,
`SIMP_PRELUDE_DIR`, `SIMP_STDLIB_MODULE_DIR`), then `SIMP_HOME` treated as the
prefix, then the executable-relative prefix. `simp --print-paths` prints the
resolved executable, prefix, runtime, include, prelude, project module root
(and where it came from), standard modules, compatibility roots, registry, and
Clang, then exits. See `doc/simp.1` (installed as `simp(1)`) and the
[documentation index](doc/README.md).

### Standard library packages

Standard packages are imported by package name and expose an alias-qualified
namespace (for example, `import system as Sys`). See the
[standard library reference](doc/STDLIB.md) for package APIs and behavior.

The repository uses one in-tree build directory: `build/`. `include`, `src`,
and `tests` each have their own `CMakeLists.txt` and are integrated by the root
project; standalone configuration is optional. The language prelude lives in
`prelude/` and standard library packages live under `stdlib/`. To configure a
component without creating another build directory in the repository, use a
temporary directory outside it, for example:

```sh
cmake -S src -B /tmp/simp-compiler-build
cmake --build /tmp/simp-compiler-build
```

Standalone component executables and libraries stay under that external build
directory. `tests/functional` contains source fixtures, not a separate
buildable component. The root-integrated build puts both executables and its
front-end archive in the project-root `bin/` and `lib/`, respectively.

### Adding a test

Compiler integration tests are discovered from individual
`tests/cases/<test-name>.cmake` files. Add a `positive_*.simp` or
`negative_*.simp` source in `tests/functional/positive/` or `negative/`,
then add a case file setting `CASE_NAME` (the CTest name) and `CASE_FIXTURE`
(the source basename). For a successful compile/run case, set
`CASE_EXPECTED_OUTPUT_FILE` to a neighboring `.stdout` file containing the
exact output (including the final newline); for a rejected program, set
`CASE_EXPECTED_DIAGNOSTIC` to the expected diagnostic regex instead.
Optional case flags include `CASE_REQUIRE_GC_ROOTS`,
`CASE_REQUIRE_VIRTUAL_DISPATCH`, `CASE_EXPECT_WARNING`,
`CASE_EXPECT_RUNTIME_FAILURE`, `CASE_EXPECT_RUNTIME_DIAGNOSTIC`, and
`CASE_MODULE_REGISTRY`. Module cases set `CASE_MODULE_ROOT` (a directory under
`tests/functional/modules/` copied into a private module root) and
`CASE_MODULE_ROOT_SOURCE` (`cli` for `-M`, the default; `env`, `default`,
`stdlib`, or the deprecated `package-path`/`package-env`);
`CASE_MODULE_ROOT_MISSING` leaves the selected root absent and
`CASE_MODULE_DECOY` copies a fixture into every lower-precedence root to prove
it is ignored. `CASE_ARGUMENTS` adds compiler arguments, `CASE_NO_RUN` only
runs the compiler, `CASE_NO_SOURCE` omits the source input, and
`CASE_EXPECT_COMPILE_OUTPUT`/`CASE_REJECT_COMPILE_OUTPUT` list regexes over the
compiler's output. Expectations may use the `<MODULE_ROOT>`, `<PROJECT_DIR>`,
and `<WORK_DIR>` placeholders. Every case runs in its own work directory with
the `SIMP_*` path variables cleared. See an existing case file for the
relevant pattern.
`CASE_DEBUG_INFO` adds a `-g` case that verifies its IR and executable DWARF
data, then checks a debugger breakpoint and local when GDB or LLDB is
installed. Reconfigure to discover newly added cases; no central test list
needs editing.

For parser/semantic functional checks in `simp_tests`, put a
`<fixture>.simp.json` file next to the `.simp` source with `friendly_name`,
`expected_diagnostic` (empty for a valid program), and `enabled` boolean
fields. Focused C++ structural assertions belong in a `tests/test_*_cases.cpp`
group using `TestGroupRegistration` from `test_cases.hpp`; CMake discovers
these groups. CLI integration fixtures live under `tests/functional/cli/`,
and their specialized shared runners accept a `CASE` selector so each
scenario has an independent CTest result.

`-v` raises verbosity and may be repeated or grouped: `-v` reports compiler
phases, `-vv` also prints every resolved path and the exact Clang commands,
and `-vvv` adds per-phase timings (all on stderr). `--verbosity=N` sets the
same level directly; levels above 3, and combining it with `-v`, are rejected.
`-t`/`--trace` selects `scanner`, `parser`, `ast`, or `symbols` tracing;
targets are case-insensitive and may be comma-separated or repeated
(`-t parser,symbols -t ast`). The `ast` and `symbols` targets print the parsed
AST and the semantic symbol table to stdout. `--check-only` runs parsing and
semantic checks without code generation. The former `--verbose`,
`--trace-parser`, `--dump-ast`, and `--dump-symbols` options have been removed.
`--path DIR` (`-p`) adds one or more
directories, including colon-separated lists, to the textual-include search
path; includes still prefer the directory of the including source. Use
`--max-include-depth N` to set the maximum nested textual include depth
(default: 16). `--help` (`-h`) prints the registered options, and `--version`
(`-V`) prints the compiler version. LLVM IR is compiled to a native executable
by the installed Clang driver; `--emit-llvm FILE` additionally saves the
combined program IR. Pass `-g` to include DWARF debug information in the
generated executable and emitted IR. Pass any number of `.simp` sources and
`.o`/`.obj` files; all Simple sources are analyzed together, so declarations
and reopened namespaces can be shared across those files. Exactly one source
in the set must contain `start`. Duplicate declarations (including symbols
and namespace conflicts) are diagnosed by the compiler. Imports in those
sources share the compilation-unit alias scope; module namespaces remain
separate from local namespaces.

Executables default to `./<first-source-basename>` (or `./a.out` for object-only
links); `-o FILE` selects another path. `-c` compiles the supplied Simple
sources together to one relocatable object without linking. The resulting
object includes the single program entry and can be linked later with
`simp program.o -o program`; object-only linking adds the compiler runtime.
Without `-o`, compile-only writes `./<first-source-basename>.o`.
`-L DIR` adds a linker search directory and `-l NAME` links `libNAME` in normal
Clang driver order. The `CC` environment variable can select a compiler-driver
executable in place of the Clang path configured at build time; it must name a
single executable (not a command with extra flags). For example:

```sh
./bin/simp src/helpers.simp src/main.simp -o bin/program -L lib -lmylibrary
./bin/simp -c src/helpers.simp src/main.simp -o build/program.o
./bin/simp build/program.o -o bin/program
```

For a single-source invocation, the default remains
`./<input-basename>`. For example, running
`../bin/simp ../tests/functional/positive/positive_gc_object_graph.simp` from
`build/` creates `build/positive_gc_object_graph`.
The emitted IR uses the GC runtime ABI; link it manually with the runtime
archive, for example:

```sh
clang -Wno-override-module -x ir build/program.ll -x none \
  lib/simp/libsimp_runtime.a -o bin/program
```
The compiler expands top-level textual includes independently for each source
input and reports source-located lexer, parser, and semantic errors.

### Debugging

Compile with `-g` and use the generated executable directly with GDB or LLDB:

```sh
./bin/simp -g tests/functional/positive/positive_debug_info.simp \
  -o build/positive_debug_info
gdb -q build/positive_debug_info
# or: lldb build/positive_debug_info
```

The debug information maps generated machine instructions back to Simple
source lines and can expose in-scope Simple local variables when their
locations are available. Debugger visibility is not guaranteed for every
value or every point in a program: compiler-generated temporaries and
optimized-out or out-of-scope locals are not inspectable, and class fields
are not necessarily shown as source-level members. This is standard debugger
support for the generated executable, not an IDE integration.

## Implemented subset

- Exactly one top-level `start { ... }` block is required. Duplicate or missing
  blocks are errors. Top-level and namespaced `class` declarations and
  qualified out-of-line method definitions are supported before `start`;
  free-standing function declarations are not.
- `namespace Name { ... }` declares a namespace with a single identifier;
  dotted names are not valid declaration syntax. Write nested namespaces by
  physically nesting `namespace` blocks, and empty namespace bodies are legal.
  Repeated paths in one compilation unit (including top-level
  `include "path"` files) contribute to the same namespace. Qualified and
  unqualified class references resolve the first component outward through
  enclosing namespaces and each later component strictly as a child symbol;
  namespaces do not inject names into other scopes.
- `include "path"` textually includes a source file at its top-level directive.
  Relative paths are resolved from the including file, and each canonical file
  is included at most once per compilation unit. Include depth is limited to 16.
- Reserved keywords are case-insensitive: `start`, `int`, `strg`, `if`,
  `else`, `while`, `print`, `class`, `super`, `null`, `return`, `void`,
  `raise`, `try`, `except`, `finally`, `for`, `in`, `public`, `protected`,
  `private`, `virtual`, `from`, `list`, `dict`, `namespace`, `include`,
  `import`, `as`, `is`, `type`, and `any`.
  `list` and `dict` are the collection type keywords. `array` and `map` are
  ordinary identifiers, not collection type aliases.
  Every capitalization is reserved.
- Use `strg` for the managed string type; lowercase `string` is no longer
  reserved and may be used as an identifier.
- `int`, `strg`, class-reference, `list`, and `dict` declarations
  (with optional initializer), assignment, `print`, `return`, and
  base-initializer statements end at a
  newline or closing brace. Newlines inside parentheses and square brackets
  are treated as whitespace. Semicolons do not terminate statements: `;`, `#`, and `//`
  begin single-line comments, while `/* ... */` is a block comment.
  See the [language reference](doc/LANGUAGE-REFERENCE.md) and
  [grammar](doc/GRAMMAR.md) for supported constructor syntax.
- Expressions include integer and string literals, identifiers, parentheses,
  unary `+`, `-`, `!`, arithmetic `+ - * / %`, comparisons `== != < <= > >=`,
  and the boolean type-test operator `expr is TypeName`. Type tests recognize
  built-in types and class names (including qualified class names); class tests
  match the runtime class or any subclass, and tests on `any` inspect its
  runtime tag. `is` has relational-comparison precedence and binds more tightly
  than `==`/`!=`, `and`, and `or`.
  `type(expr)` returns a value of type `type` describing the exact runtime
  type, including the dynamic class behind a base reference or `any` value;
  null is reported as the type name `null`. Type values compare by exact name
  with `==`/`!=` (not by inheritance) and print that name; ordering is not
  supported.
  list and dict literals, zero-based indexing, copying list/dict slices, and
  dict operations `dictValue.contains(stringExpression)` and
  `dictValue.remove(stringExpression)`.
- `if (condition) { ... }`, unconditional `else { ... }`, and
  `while (condition) { ... }`, `for (value in listValue) { ... }`,
  `for (value in dictValue) { ... }`, and
  `for (key, value in dictValue) { ... }` execute in the LLVM backend.
  Conditions must be `bool`. An `else (condition)` form is explicitly rejected.
- `raise(Exception("message"))` throws a constructed exception object. User
  exception classes derive from `Exception`; `raise(MyError(args))` accepts
  only a constructor call whose class belongs to that hierarchy. A `try` may
  have multiple ordered `except` clauses: each `except(MyError)` catches that
  class and its subclasses, and dispatch continues to the next clause on a
  mismatch. Qualified filters such as `except(errors.MyError)` are supported.
  A final `except()` catches any remaining exception. `except() as message`
  binds a catch-all message as a read-only `strg`; `except(MyError) as error`
  binds the caught object as a read-only `Exception` reference,
  exposing its inherited `message` field. Catch-all clauses must be last,
  and a subclass clause following a matching base-class clause is rejected as
  unreachable.
  `finally { ... }` is optional and runs after normal
  completion or while an exception propagates. `try` must include `except`,
  `finally`, or both. Exceptions raised inside `except` still run its paired
  `finally`; an exception raised inside `finally` propagates outward.
- Null dereferences, use of explicitly destroyed objects, repeated destruction,
  and integer division/remainder by zero raise catchable `Exception` instances.
  An uncaught exception prints its source file, line, column, and message to
  standard error, followed by Simple source frames from the innermost method
  or constructor out through callers to `start`; C runtime frames are omitted.
  Caught exceptions do not print traces, and rethrows preserve the original
  trace. Runtime invariant failures and exceptions escaping GC finalizers
  remain fatal.
- `print(expr)` prints a scalar, string, type, or internal collection value
  followed by a newline; collection values are rendered by runtime tag (see
  below).
- Basic formatting uses a double-quoted literal followed by an expression list:
  `strg result = "value: {}"(value)`. The result is a reusable `String`
  expression, including in returns, arguments, arrays, maps, and `print`.
  Each `{}` substitutes one supported scalar, `String`, type, or internal
  dynamic value; class references without string conversion show `<object>`.
  Arguments are evaluated once in source order. Unmatched braces and
  argument-count mismatches are compile-time errors.
- Single-quoted strings are raw literals: they have no escapes and cannot be
  used with formatting arguments. Double-quoted strings support `\e` (byte
  `0x1b`, ESC), `\\`, `\"`, `\n`, `\r`, and `\t`; their source bytes must be
  valid UTF-8.
- `strg` is an alias for the prelude `String` class, with a private GC-managed
  `buffer _bytes`. Literals create objects without calling a public constructor.
  Assignment shares the object; `append(other)` mutates all aliases.
  `length` counts UTF-8 bytes, `equals(other)` compares bytes, and `==`/`!=`
  compare object identity. `byteAt`, `slice`, `insert`, `removeRange`, `clear`,
  search, prefix/suffix, split, replace, trim/strip, and ASCII case conversion are
  available as byte-based methods; invalid bounds and UTF-8-splitting ranges
  raise catchable exceptions. Unicode case mapping, code-point operations,
  and direct string index syntax are deferred. `toInt()`, `toUnsigned()`, and
  `toFloat()` parse checked numbers. Dict keys copy the bytes at insertion, so
  later mutation cannot change a stored key.
- Lists (`list`) are heterogeneous bags: a single literal such as
  `[1, "two", Node(3), null]` may freely mix ints, strings, class references,
  lists, dicts, and `null` in one collection; an empty literal `[]` is always
  allowed. Nested lists and collections are traced by the GC.
  Reading an element with `values[index]` yields an internal dynamic value —
  it does not statically know whether that slot holds an `int`, a `strg`, a
  class reference, or a dict reference. Dynamic values are not a declared
  type: they can be tested with `is`/`type`, compared with `null`, printed,
  stored in collections, or extracted into a concretely typed variable, field,
  parameter, or return value (with a runtime check). Other operations such as
  arithmetic and member access require typed extraction. Assigning
  `values[index] = expr` accepts
  any of the supported element types directly. The read-only
  `values.length` property returns an `int`. Indexing is zero-based.
  `values[start:end]` creates a new list containing the half-open range
  `[start, end)`; either bound may be omitted (`values[:end]`,
  `values[start:]`, or `values[:]`). Slices are independent shallow copies, so
  changing a copied scalar slot does not change the source (class-reference
  elements still refer to the same objects). Lists also support
  `values[start:end:step]`, including omitted bounds such as `values[::2]`;
  positive and negative steps follow Python's slice-bound normalization,
  including clamping out-of-range bounds, and a zero step raises a catchable
  runtime exception. List and buffer indexing accept negative indices, where
  `-1` selects the last element; indices still out of range after normalization
  raise catchable runtime exceptions with source locations.
- List iteration visits elements in index order and binds each element as an
  internal dynamic value; dict iteration binds an internal dynamic value alone
  or a `strg` key and dynamic value in insertion order. The binding follows
  the same use restrictions as an indexed collection read. Both forms snapshot
  their entries (including
  values) at loop start: insertion, removal, or replacement during the loop
  does not change the current iteration. The snapshot is shallow, so referenced
  objects and nested collections remain shared.
- Dicts (`dict`) are mutable heterogeneous dictionaries.
  A literal uses `{ "name": "Ada", "age": 37 }`; keys are string expressions
  and are compared by exact UTF-8 bytes (case-sensitive, without normalization).
  Reading `values[key]` yields an internal dynamic value; writing
  `values[key] = value` inserts
  a new key or replaces the existing value. Replacing a key does not change
  `values.length`, which counts distinct keys and is read-only. A missing key
  raises a source-located, catchable `dict key not found` exception. Dict
  assignment aliases the same mutable storage. Values may be ints, strings,
  class references, null, `any`, lists, or dicts; list/dict references extracted
  from `any` are runtime-checked. Dicts can contain nested dicts and lists, and
  lists can contain nested lists. Exact UTF-8 byte hashing provides expected
  constant-time lookup while a separate insertion-order sequence keeps
  iteration deterministic. Keys must have statically known type `strg`; an
  `any` value is not accepted as a key without first testing or extracting it
  to `strg`. `values.remove(key)` returns `1` when an entry was removed and
  `0` when it was absent; removing a key preserves the order of other entries,
  and reinserting it appends it. `values[start:end]` makes an independent
  shallow dict copy from the half-open insertion-order range `[start, end)`;
  either bound may be omitted, with omitted bounds defaulting to the start and
  end of the insertion-ordered entries. Buffers likewise support omitted
  bounds and return independent copies. Negative bounds on dict and buffer
  slices are normalized relative to collection length and clamped to the valid
  range. Step slices are restricted to lists. Dict indexing remains
  string-keyed.
- Equality and ordering comparisons require matching numeric types
  (`int`, `unsigned`, or `float`); equality also supports matching `bool`
  operands. Compatible class references also support `==` and `!=`, which
  compare object identity (not field values or dynamic class); an upcast
  reference compares the same base subobject as its derived reference.
  Unrelated class references cannot be compared. Any nullable type may be
  compared with `null`. Strings follow class-reference identity rules;
  lists, dicts, and `any` do not support equality with each other.
- Collection reads and loop bindings use an internal tagged dynamic value;
  the reserved keyword `any` is not permitted as a declared type (locals,
  fields, parameters, returns, or native signatures). The representation can
  carry scalars, strings, class references, lists, dicts, buffers, handles, and
  `null`. It may only be printed, tested with `is` or `type`, compared with
  `null`, extracted into a concretely typed variable/field/parameter/return
  value, or inserted into another collection. Extraction is runtime-checked
  and raises a catchable exception on a tag mismatch; class extraction
  currently requires an exact class match (or a null reference). `is` on a
  class target matches subclasses, while `type(value)` reports the exact
  dynamic type. Dynamic values have no member access and cannot be used in
  arithmetic or other operators. Printing dispatches on the runtime tag;
  object references print a fixed `<object>` placeholder (or `null`), since
  user-defined `toString()` dispatch is not implemented. Class-reference and
  collection values reachable through lists and dicts are traced by the GC.
- `;`, `#`, and `//` line comments, `/* ... */` block comments, and basic
  double-quoted escapes (`\e`, `\\`, `\"`, `\n`, `\r`, `\t`) are accepted.
  `\e` produces byte `0x1b` (ESC). Single-quoted strings have no escape
  processing.
- Semantic analysis resolves lexical local names, rejects use before
  initialization and undeclared identifiers, checks `int`/`strg`
  initialization and assignment types, validates integer literal range, and
  requires integer conditions. Definite initialization across `if` branches
  and loops is conservative.
- A small class subset is supported: top-level `class` declarations with
  `int`, `strg`, `list`, `dict`, or class-reference fields; class-named constructors
  overloaded by parameter types;
  typed methods; `Class(args)` construction/allocation; nullable class-reference
  variables; field access/assignment; method calls; and direct `return`
  statements at the end of methods. Inheritance uses `class Child : Base` or
  `class Diamond : Left, Right`; `virtual` marks a shared base at any depth,
  for example `class Left : virtual Root` or
  `class Root : virtual Ancestor`.

  A regular method can be declared without a body in its class and defined
  later as `ReturnType Class.method(params) { ... }`. The signatures must
  match, and every bodyless method needs exactly one matching definition.
  `from "<symbol>"` replaces the out-of-line body to bind a C implementation;
  callers still use ordinary `receiver.method(args)` syntax.

  Constructors with distinct parameter signatures may be overloaded. Calls
  prefer exact argument-type matches over class-to-base conversions; equally
  good conversions (including `null` for multiple class-reference parameters)
  are diagnosed as ambiguous. Constructors may also be declared in-class and
  defined out-of-line with `Class Class.Class(params) { ... }`. Native-bound
  constructors are not supported.

  ```simple
  class Counter {
      int value
      Counter(int initial) {
          value = initial
      }
      int add(int amount) {
          value = value + amount
          return value
      }
  }
  start {
      Counter counter = Counter(40)
      print(counter.add(2))
      print(counter.value)
  }
  ```

  Constructors are named exactly after their class. A class may declare a
  zero-argument `void destroy()` method; an explicit `object.destroy()` warns,
  invokes the destructor chain once, and marks the object unusable. Further
  object use or another destruction attempt raises a catchable runtime
  exception. Destructors execute from the most-derived class through
  non-virtual bases in reverse construction order, then shared virtual bases
  once in reverse virtual-base construction order. Explicit destruction and GC
  finalization use the same chain. A destructor error cannot make an object
  eligible for a second destructor run; explicit destruction continues through
  the remaining bases before propagating the first destructor exception.
  Finalizer allocation aborts.
  Direct non-virtual bases have distinct subobjects in declared order; qualified
  field and method paths select a specific subobject. A virtual base and its
  own base graph are shared by every path in the complete object, so a
  transitive diamond can share both an intermediate virtual base and its
  virtual ancestors. Ambiguous inherited fields must be qualified, for example
  `diamond.Left.Root.Ancestor.value`; unqualified ambiguous fields or methods
  are compile-time errors. Base-constructor and virtual-base initializer syntax,
  ordering, and validation are described in the
  [language reference](doc/LANGUAGE-REFERENCE.md).
  Implicit upcasts adjust to the unique accessible base
  subobject; ambiguous conversions are errors. Virtual dispatch uses per-view
  metadata and adjusts `this` to the selected implementation's subobject.
  The first declared base retains the primary designation and first layout
  position, but construction and dispatch support every direct base.
  Overrides must exactly preserve inherited return and parameter types.
  Inherited field redeclaration and incompatible overrides are unsupported.

  Each direct base may be marked `public`, `protected`, or `private`; omitted
  visibility defaults to `public` for compatibility with the current subset.
  The `virtual` modifier may precede or follow visibility, as in
  `public virtual Root` or `virtual public Root`.
  External code may access inherited fields/methods only through public base
  paths. Protected/private paths are available to members of the class that
  declares the path but are hidden from `start` and unrelated classes. This
  applies the standard access transformation: public inheritance preserves
  member access, protected inheritance maps inherited public/protected members
  to protected, and private inheritance maps them to private. A base-private
  member remains inaccessible through inheritance.

  Class bodies support `public:`, `protected:`, and `private:` sections; the
  default section is public. These sections apply to subsequent fields,
  methods, constructors, and destructor declarations. Private members are
  accessible only in their declaring class; inherited
  private members are not accessible to further-derived classes. Protected
  members are accessible in their declaring class and derived-class method
  bodies. Constructors are
  checked at object creation and during base-constructor initialization; destructor access is
  checked on explicit invocation. Protected access currently checks the enclosing class
  relationship but not C++'s additional receiver-expression restriction for
  protected members. There are no friends, overloads, or access labels on
  individual declarations outside the section syntax.

  Objects have stable, non-moving addresses. The root object and each base
  subobject have a metadata header and link to the containing allocation, with
  direct non-virtual bases embedded in declared order before the class's fields.
  Each complete object stores one physical instance of each virtual base class
  reachable through its inheritance graph; all virtual paths resolve to that
  canonical subobject. Transitive virtual bases are included in the same layout,
  dispatch metadata, and precise GC reference offsets. Metadata
  contains the dynamic class name/field count, per-view virtual method tables,
  object size, and compiler-generated offsets for class-reference fields;
  instances do not contain method copies. Class references may be `null`;
  dereferencing null raises a catchable runtime exception. Newly allocated
  fields are zero-initialized
  before the constructor runs. Method overloading and default field initializer
  syntax are unsupported.

The parser is recursive descent and produces an AST that can be dumped with
`-t ast`. Lexer, parser, and CLI diagnostics include file, line, and column.
Parser tracing is available with `-t parser`; `-t symbols` displays
the declarations and initialization state seen by semantic analysis.

## Executable backend subset

The backend emits textual LLVM IR using opaque pointers, then the configured
Clang executable compiles and links it. It supports scalar and string
declarations/assignments, heterogeneous `list` and `dict` collections (with
internal tagged values for dynamic elements and values), integer
expressions and comparisons, integer
`if`/`else` and `while`, `raise`/typed `try`/`except`/`finally`,
single-value scalar, string, type, or internal dynamic-value printing, and the
`{}` formatting form described above. It also supports object
layout/allocation/constructor/method/field operations for classes
and single- and multiple-inheritance layouts, including transitive shared
virtual bases. Base-path field access distinguishes repeated non-virtual
subobjects while paths to a shared virtual base select one instance. Base constructors,
unique-subobject upcasts, and virtual dispatch work through primary, secondary,
and virtual base views.
Method-table slots are inherited in stable order and an override replaces its
inherited slot; per-subobject dispatch thunks adjust the receiver before
invoking the selected implementation. `String` objects own a traced `buffer`
of UTF-8 bytes; `fwrite` uses the borrowed bytes and length without requiring
a terminator. The program entry returns zero.

Generated functions register and pop explicit root frames. Descriptors list
only object-reference stack slots (including parameters, `this`, locals, and
object-valued temporaries); the collector never scans arbitrary stack words.
The C-compatible runtime performs a stop-the-world, non-moving mark/sweep
collection before each class or array allocation and follows generated
reference-field offsets plus tagged class references in array elements. This prototype is single-threaded. Root slots are
kept for the whole function, so dead locals/temporaries may retain objects
until their frame returns. The runtime unit test checks root-frame misuse,
survival through an object-reference field, and reclamation after the last
root is removed; an executable stress fixture allocates a linked object graph
through repeated collections. Inheritance integration tests also call
overridden methods through primary and secondary base-typed references, verify
nested reference tracing and construction-failure behavior, and check
reverse-order destruction through secondary subobjects.

The parser and semantic analyzer accept more syntax than the backend executes.
String identity comparisons, reusable formatted expressions, and the documented
byte-based String methods are supported; string index syntax and Unicode-aware
text operations remain deferred.
Exception handling uses direct LLVM `setjmp` calls
paired with the C runtime's `longjmp`; generated frames snapshot and restore
the precise GC root chain and explicitly running destructor chain before
catching. Catch clauses may be typed or untyped; an optional read-only String
binding exposes the message, and uncaught diagnostics include the original
raise or runtime-check location and stack trace. Returns from inside
`try`/`except`/`finally` are rejected. Exceptions
cannot escape a GC finalizer; finalizer exceptions and collector invariant
failures remain fatal. This is a single-threaded host-C ABI implementation,
not LLVM landing-pad or cross-platform exception support. There is no string
concatenation, object-to-string conversion, or code-point-aware operation.
Caught exception messages are retained until process exit so a bound string
copied into a longer-lived variable or object field remains valid.
The collector traces arrays and maps, including tagged class references and
nested collection references reachable through `any` values, in
addition to object fields. It has finalizers for inherited classes, but no weak references, multithreading,
incremental/concurrent collection, or configurable allocation threshold.
Generated roots conservatively include every object-typed slot in a function,
but do not scan non-reference values or the native stack. This small runtime
has stress/unit coverage but is not a production-validated memory manager.
Multiple inheritance uses deterministic layout; repeated non-virtual ancestors
remain distinct, while transitive virtual ancestors are shared. Method
overloading and reflection are also unsupported. Virtual-base constructors run
once in depth-first, left-to-right order, initializing a virtual base's virtual
ancestors first, before non-virtual base constructors. Destruction runs
non-virtual bases in reverse order, then shared virtual bases once in reverse
virtual-base construction order. If
virtual-base construction throws, the partial complete object is marked failed
and its destructor chain is not run.

## Out-of-line methods and native bindings

Methods remain class members: a method signature is declared inside its class,
then its body may be supplied later, qualified with the class name. A C
binding uses `from "<symbol>"` instead of the out-of-line body. There is no
top-level free-function declaration form and no `extern` keyword in this
syntax.

```simple
class Native {
    int absolute(int value)
    int stringLength(strg text)
    strg stringIdentity(strg text)
    list identityArray(list items)
}

int Native.absolute(int value) from "simp_method_demo_abs"
int Native.stringLength(strg text) from "simp_method_demo_string_length"
strg Native.stringIdentity(strg text) from "simp_method_demo_string_identity"
list Native.identityArray(list items) from "simp_method_demo_identity"

class Doubler {
    int compute(int x)
}

int Doubler.compute(int x) {
    return x * 2
}

start {
    Native native = Native()
    print(native.absolute(0 - 7))       # 7; C shim calls libc abs()
    print(native.stringLength("hello")) # 5
    print(native.stringIdentity("hello")) # hello
    list numbers = [1, 2, 3]
    print(native.identityArray(numbers).length) # 3
    print(Doubler().compute(5)) # 10
}
```

- Exact forms:
  - In-class declaration: `<returnType> <method>(<paramType> <paramName>, ...)`
    with no body.
  - Out-of-line Simple body:
    `<returnType> <Class>.<method>(<paramType> <paramName>, ...) { ... }`
  - C binding:
    `<returnType> <Class>.<method>(<paramType> <paramName>, ...) from "<symbol>"`
- A declaration must be paired with exactly one out-of-line definition.
  The return type, parameter count, and parameter types must match; method
  calls use ordinary `receiver.method(args)` syntax and validate argument
  count/types against the class declaration. Constructors/destructors remain
  defined in-class and cannot use `from`.
- An out-of-line definition must appear in exactly the same lexical namespace
  scope as its class declaration, and in the same compilation unit.
- Native-bound methods are ordinary class methods to their callers. The
  external implementation detail is not exposed at the call site. The
  compiler emits an ordinary Simple method/dispatch entry as a wrapper around
  the external symbol; the wrapper passes the implicit receiver pointer as
  the **first C ABI argument**, followed by explicit parameters.
- ABI mapping: `int` is C `int64_t` (`i64`); `String`/`strg`, `list`, `dict`,
  and other class references are single opaque pointers; `void` is C `void`.
  This **breaks the earlier `SimpString {data,length}` native ABI**: C code
  must use `simp_string_bytes(object, &data, &length)` to borrow non-NUL-
  terminated bytes and cannot retain that view across a resize. `any` is
  rejected in native signatures. Inline C may capture a `buffer` as
  `SimpBuffer **`, and use `simp_buffer_resize`/`simp_buffer_set` on `*slot`;
  never realloc GC-managed objects or access a private String field from
  Simple. Invalid UTF-8 bytes passed to String construction/append raise;
  arbitrary buffer-to-String conversion and direct byte mutation are deferred.
- Argument expressions reuse normal call evaluation and explicit GC rooting:
  already-evaluated managed arguments remain rooted while later arguments
  execute, and the receiver/parameters are rooted in the generated wrapper.
  Managed return pointers are checked and rooted before the wrapper returns.
  C code that allocates managed objects must use the runtime's root-frame API
  for its own temporary references.
- A missing `<symbol>` is diagnosed by the linker at link time, not by
  semantic analysis. `tests/functional/positive/positive_extern_functions.simp`
  exercises `from` bindings for integer, strg argument/return, list, and
  class-reference values; its bundled C shims include a call to libc `abs()`.
- Compiled imports use `import <package-or-module> as <symbol>`. Packages live
  at `<search-root>/<name>/<version>/simp-package.toml`; their Simple source,
  one designated class or namespace export, exact package dependencies, and
  native linker inputs are described by that manifest. For example:

  ```toml
  [package]
  name = "sqlite"
  version = "1.0.0"
  source = "src/sqlite.simp"
  export = "namespace:SQLite"

  [dependencies]
  sys = "=1.0.0"

  [link]
  libraries = ["simp_sqlite", "sqlite3"]
  library-paths = ["lib"]
  ```

  To author one, put the manifest, `src/sqlite.simp`, and native shim source
  under `sqlite/1.0.0/`. Declare methods in the Simple class and bind each
  method out of line with `from "c_symbol"`; the C shim receives the opaque
  Simple receiver as its first argument. Build the shim/archive separately
  into `sqlite/1.0.0/lib/` (and install upstream native dependencies using the
  platform's normal build tools). Put the package under the project module
  root (`<project-root>/modules/`) or pass another root with `-M`:

  ```c
  int simp_sqlite_version(void *receiver) {
      (void)receiver;
      return 1;
  }
  ```

  ```sh
  clang -c sqlite/1.0.0/native/sqlite_shim.c -o build/sqlite_shim.o
  ar rcs sqlite/1.0.0/lib/libsimp_sqlite.a build/sqlite_shim.o
  ./bin/simp -M ./packages app.simp -o bin/app
  ```

  A consumer writes `import sqlite as DB` and calls the imported class or
  namespace through `DB`; the package resolver adds its native libraries
  during linking. This keeps the language-facing API in Simple while leaving
  native library construction to the package author.

  Package versions use SemVer; direct imports select the highest stable
  version in the first module root containing that package, and dependencies
  use exact `=VERSION` pins. The resolver
  detects conflicting pins and dependency cycles. It adds package libraries
  and search   directories to the existing Clang link step; conflicting package-local
  search locations for the same native library name are diagnosed. The
  compiler does not run package build scripts, so package authors build native
  shims and libraries separately. Package source methods still use the existing
  `from "c_symbol"` binding and ordinary Simple method calls.

  Packages live at `<module-root>/<name>/<version>/simp-package.toml`. The
  module roots searched are, in order:

  1. The canonical project module root: `-M DIR`/`--module-dir DIR`, else
     `SIMP_MODULE_DIR`, else `<project-root>/modules`. The project root is
     currently the directory containing the first `.simp` input (the current
     directory when there is none). An explicitly selected root that does not
     exist is an error; a missing default root is simply skipped.
  2. The standard modules shipped with the compiler
     (`${CMAKE_INSTALL_DATADIR}/simp/modules`, overridable with
     `SIMP_STDLIB_MODULE_DIR`).
  3. Deprecated compatibility roots: `--package-path DIR` values, then
     `SIMP_PACKAGE_PATH`. Each prints a deprecation warning when used. The
     former implicit `./.simp/packages` and XDG/home roots are no longer
     searched.

  The first root containing a package shadows lower-priority roots; version
  selection occurs only among that root's installed versions. `-c`
  writes adjacent `.simp-link` metadata (package linker inputs, or an empty
  sidecar for package-free objects); `simp program.o -o program` reads it
  automatically. The standard module directory is reserved for the planned
  `sys`, JSON, regex, datetime, networking, and SQLite packages. A project
  manifest that declares the project root and its module directory is
  deferred; until then the first source's directory serves as the project
  root.

  The legacy six-column `simp-modules.tsv` catalog (`./simp-modules.tsv`, or
  `SIMP_MODULE_REGISTRY`) remains the final compatibility fallback; a warning
  is printed whenever an import is resolved through it. Package manifests are
  preferred when the same import name is present. When an import cannot be
  resolved, the diagnostic lists every normalized module root and registry
  path searched, marking missing ones `[not found]`. The legacy
  registry's version/dependency fields remain metadata only. Imported
  declarations remain available only through aliases: a namespace alias can
  qualify nested types (`Net.Http.Client().get("/")`), while a class alias is
  constructed directly (`Client(args).method()`). See
  `SIMPLE-LANGUAGE-NOTES.md` for the complete package format and rules.



LLVM 22.1.8, CMake 3.31.6, GCC 14.2, and Clang 22.1 were available when this
prototype was extended. The implementation emits textual LLVM IR and invokes
Clang; it does not link the LLVM C++ API or provide a configurable LLVM
optimization pipeline. Building the compiler requires Clang on `PATH`; the
current driver launches it through the host POSIX shell. Full language type
checking and name-resolution rules, OOP beyond the supported single- and
multiple-inheritance slices (including access to protected
base members from further-derived classes), production GC features, package
build scripts, version ranges/lockfiles, platform-specific native-link rules,
inline C, GTK, package manager, and IDE remain deferred. Collection deletion
remains unimplemented. Namespace, include, compiled source-module
imports, package resolution, and package-native linking are implemented; other
design-note proposals may still be unsupported.
