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

The root build writes executables to project-root `bin/` and static, shared, or
module libraries to project-root `lib/`. On Linux the archives are
`lib/libsimp_frontend.a` and `lib/libsimp_runtime.a`.

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
code generation). `--max-include-depth N` sets the maximum nested textual
include depth (default: 16). LLVM IR is compiled to a native executable by the
installed Clang driver; `--emit-llvm FILE` additionally saves the generated IR.
Executables default to `./<input-basename>` in the compiler's current working
directory; `-o FILE` selects another path. For example, running
`../bin/simp ../tests/functional/positive/positive_gc_object_graph.simp` from `build/`
creates `build/positive_gc_object_graph`.
The emitted IR uses the GC runtime ABI; link it manually with the runtime
archive, for example:

```sh
clang -Wno-override-module -x ir build/program.ll -x none \
  lib/libsimp_runtime.a -o bin/program
```
The compiler reads one source file, expands top-level textual includes, and
reports source-located lexer, parser, and semantic errors.

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
- Reserved keywords are case-insensitive: `start`, `int`, `string`, `if`,
  `else`, `while`, `print`, `class`, `super`, `null`, `return`, `void`,
  `raise`, `try`, `except`, `finally`, `for`, `in`, `public`, `protected`,
  `private`, `virtual`, `from`, `map`, `dict`, `namespace`, `include`,
  `import`, and `as`.
  `dict` is an alias for the `map` type.
  Every capitalization is reserved.
- `int`, `string`, class-reference, `array`, `map`/`dict`, and `any` declarations
  (with optional initializer), assignment, `print`, `return`, and
  `super.Base(...)` statements end at a
  newline or closing brace. Newlines inside parentheses and square brackets
  are treated as whitespace. Semicolons do not terminate statements: `;`, `#`, and `//`
  begin single-line comments, while `/* ... */` is a block comment.
- Expressions include integer and string literals, identifiers, parentheses,
  unary `+`, `-`, `!`, arithmetic `+ - * / %`, comparisons `== != < <= > >=`,
  array and map literals, zero-based indexing, copying array/map slices, and
  map operations `mapValue.contains(stringExpression)` and
  `mapValue.remove(stringExpression)`.
- `if (condition) { ... }`, unconditional `else { ... }`, and
  `while (condition) { ... }`, `for (value in arrayValue) { ... }`,
  `for (value in mapValue) { ... }`, and
  `for (key, value in mapValue) { ... }` execute in the LLVM backend.
  Conditions are integer expressions; zero is false and nonzero is true. An
  `else (condition)` form is explicitly rejected.
- `raise "message"` throws a runtime exception. `try { ... } except { ... }`
  catches any exception. `except error { ... }` additionally binds its message
  as a read-only `string` named `error`, visible only in that handler.
  `finally { ... }` is optional and runs after normal
  completion or while an exception propagates. `try` must include `except`,
  `finally`, or both. Exceptions raised inside `except` still run its paired
  `finally`; an exception raised inside `finally` propagates outward.
- Null dereferences, use of explicitly destroyed objects, repeated destruction,
  and integer division/remainder by zero raise catchable runtime exceptions.
  An uncaught exception prints its source file, line, column, and message to
  standard error, then aborts (nonzero process status). Runtime invariant
  failures and exceptions escaping GC finalizers remain fatal.
- `print(expr)` prints one `int`, `string`, or `any` value followed by a
  newline; printing `any` dispatches on its runtime tag (see below).
- Basic formatting uses a double-quoted literal followed by an expression list:
  `print("value: {}"(value))`. Each `{}` substitutes exactly one integer
  expression. Only `{}` placeholders are supported; unmatched braces,
  non-integer substitutions, and argument-count mismatches are errors.
- Single-quoted strings are raw literals: they have no escapes and cannot be
  used with formatting arguments. Double-quoted strings support `\\`, `\"`,
  `\n`, `\r`, and `\t`; their source bytes must be valid UTF-8.
- String values are represented as a pointer and byte length. They are not
  NUL-terminated. Direct string printing and formatting write their UTF-8 bytes
  by explicit length. No concatenation, indexing, string comparisons, or
  code-point operations are implemented.
- Arrays (`array`, with `list` accepted as an alias keyword for the exact same
  type) are heterogeneous bags: a single literal such as
  `[1, "two", Node(3), null]` may freely mix ints, strings, class references,
  arrays, maps, and `null` in one collection; an empty literal `[]` is always
  allowed. Nested arrays and collections are traced by the GC.
  Reading an element with `values[index]` yields the explicit dynamic `any`
  value type — it does not statically know whether that slot holds an `int`,
  a `string`, a class reference, or a map reference. Assigning
  `values[index] = expr` accepts
  any of the supported element types directly. The read-only
  `values.length` property returns an `int`. Indexing is zero-based.
  `values[start:end]` creates a new array containing the half-open range
  `[start, end)`; it is an independent shallow copy, so changing a copied
  scalar slot does not change the source (class-reference elements still refer
  to the same objects). Invalid indices and slice bounds raise catchable
  runtime exceptions with source locations.
- Array iteration visits elements in index order and binds each element as
  `any`; map iteration binds an `any` value alone or a string key and `any`
  value in insertion order. Both forms snapshot their entries (including
  values) at loop start: insertion, removal, or replacement during the loop
  does not change the current iteration. The snapshot is shallow, so referenced
  objects and nested collections remain shared.
- Maps (`map`, with `dict` as an alias) are mutable heterogeneous dictionaries.
  A literal uses `{ "name": "Ada", "age": 37 }`; keys are string expressions
  and are compared by exact UTF-8 bytes (case-sensitive, without normalization).
  Reading `values[key]` yields `any`; writing `values[key] = value` inserts
  a new key or replaces the existing value. Replacing a key does not change
  `values.length`, which counts distinct keys and is read-only. A missing key
  raises a source-located, catchable `map key not found` exception. Map
  assignment aliases the same mutable storage. Values may be ints, strings,
  class references, null, `any`, arrays, or maps; array/map references extracted
  from `any` are runtime-checked. Maps can contain nested maps and arrays, and
  arrays can contain nested arrays. Exact UTF-8 byte hashing provides expected
  constant-time lookup while a separate insertion-order sequence keeps
  iteration deterministic. Keys must have statically known type `string`; an
  `any` value is not accepted as a key without an implemented type-test or
  extraction. `values.remove(key)` returns `1` when an entry was removed and
  `0` when it was absent; removing a key preserves the order of other entries,
  and reinserting it appends it. `values[start:end]` makes an independent
  shallow map copy from the half-open insertion-order range `[start, end)`.
- Equality and ordering comparisons are intentionally limited to `int`.
  Strings, objects, arrays, maps, and `any` do not support `==` or `!=`.
- `any` is the explicit dynamic/tagged value type: it can hold an `int`, a
  `string`, a class reference, `null`, or a map reference; map lookups can also
  carry array references through it. It can be declared directly
  (`any value = ...`) or produced implicitly by indexing into an array or map.
  Array expressions still cannot be assigned directly to `any`. `any` has no
  members of its own — assign it to a concretely typed variable, field, or
  parameter to extract its value. Extraction is runtime-checked: it
  raises a catchable exception if the dynamic value's tag does not match the
  requested type, or (for class targets) if its exact runtime class does not
  match the requested class. Map and array tags are distinct and checked on
  extraction. There is no covariant/polymorphic downcast
  support — only an exact class match (or a `null` reference) is accepted.
  Printing an `any` dispatches on its runtime tag: an `int` or `string`
  payload prints its value; an object reference prints a fixed `<object>`
  placeholder (or `null`), including map and array references, since
  user-defined `toString()` dispatch is not implemented.
- The array/map/`any` subset is deliberately bounded: direct array-to-`any`
  conversion is unsupported (an array can still be carried through a map value
  or a collection element). Append/resize operations, omitted slice bounds,
  slice steps, and non-integer equality are unsupported.
  Class-reference and collection values reachable through arrays, maps, and
  `any` are traced by the GC.
- `;`, `#`, and `//` line comments, `/* ... */` block comments, and basic
  double-quoted escapes (`\\`, `\"`, `\n`, `\r`,
  `\t`) are accepted. Single-quoted strings have no escape processing.
- Semantic analysis resolves lexical local names, rejects use before
  initialization and undeclared identifiers, checks `int`/`string`
  initialization and assignment types, validates integer literal range, and
  requires integer conditions. Definite initialization across `if` branches
  and loops is conservative.
- A small class subset is supported: top-level `class` declarations with
  `int`, `string`, `array`, `map`, `any`, or class-reference fields; one class-named constructor;
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
  are compile-time errors. `super.Base(args)` initializes a direct non-virtual base;
  required non-virtual base constructors must be called once, in declared-base
  order, after any virtual-base initializers and before the constructor body.
  A most-derived constructor supplies a virtual base's arguments with
  `super.virtual Base(args)`, for example `super.virtual Root(seed)`. Such
  initializers must be direct leading statements, precede direct-base calls,
  and follow depth-first, left-to-right virtual-base construction order, with a
  virtual base's own virtual ancestors initialized first. Each parameterized
  virtual base must be initialized exactly once by the complete object's
  constructor; a missing initializer is an error when that class is constructed.
  A no-argument virtual base constructor is called automatically when omitted. Duplicate
  initializers, wrong argument counts or types, and names that are not virtual
  bases are errors. A constructor in a class that is itself used as a base in
  the program may not declare `super.virtual`; only most-derived classes may
  do so. Intermediate constructors cannot forward or override those arguments.
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
  checked at object creation and at `super.Base(...)`; destructor access is
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
  before the constructor runs. Constructor overloading, method overloading,
  and default field initializer syntax are unsupported.

The parser is recursive descent and produces an AST that can be dumped with
`--dump-ast`. Lexer, parser, and CLI diagnostics include file, line, and column.
Parser tracing is available with `--trace-parser`; `--dump-symbols` displays
the declarations and initialization state seen by semantic analysis.

## Executable backend subset

The backend emits textual LLVM IR using opaque pointers, then the configured
Clang executable compiles and links it. It supports integer and string
declarations/assignments, heterogeneous `array` and `map` collections (with `any` as
the explicit dynamic element/value type), integer
expressions and comparisons, integer
`if`/`else` and `while`, `raise`/catch-all `try`/`except`/`finally`,
single-value integer, string, or `any` printing, and the
limited `{}` integer formatting form described above. It also supports object
layout/allocation/constructor/method/field operations for classes
and single- and multiple-inheritance layouts, including transitive shared
virtual bases. Base-path field access distinguishes repeated non-virtual
subobjects while paths to a shared virtual base select one instance. Base constructors,
unique-subobject upcasts, and virtual dispatch work through primary, secondary,
and virtual base views.
Method-table slots are inherited in stable order and an override replaces its
inherited slot; per-subobject dispatch thunks adjust the receiver before
invoking the selected implementation. Strings
store UTF-8 bytes plus an explicit byte count; `fwrite` writes those bytes
without requiring a terminator. The program entry returns zero.

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
String comparisons and other non-integer formatted values produce precise
backend/semantic errors. Exception handling uses direct LLVM `setjmp` calls
paired with the C runtime's `longjmp`; generated frames snapshot and restore
the precise GC root chain and explicitly running destructor chain before
catching. Catch clauses are untyped; an optional read-only string binding
exposes the message, and uncaught diagnostics include the original raise or
runtime-check location (there is no stack trace). Returns from inside
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
    int stringLength(string text)
    string stringIdentity(string text)
    array identityArray(array items)
}

int Native.absolute(int value) from "simp_method_demo_abs"
int Native.stringLength(string text) from "simp_method_demo_string_length"
string Native.stringIdentity(string text) from "simp_method_demo_string_identity"
array Native.identityArray(array items) from "simp_method_demo_identity"

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
    array numbers = [1, 2, 3]
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
- ABI mapping is currently x86-64 SysV: `int` is C `int` (`i32`); `string`
  arguments scalarize to `(ptr, uint64_t length)` and string results use the
  corresponding two-scalar aggregate; `array`, `map`, and class references
  are single opaque pointers; `void` is C `void`. `any` is rejected for a
  native-bound method because the backend does not implement its target-
  specific aggregate ABI lowering. Strings are length-prefixed, not
  NUL-terminated. The matching C `SimpString` structure and shim prototypes
  are in `include/simp/RuntimeGc.h`.
- Argument expressions reuse normal call evaluation and explicit GC rooting:
  already-evaluated managed arguments remain rooted while later arguments
  execute, and the receiver/parameters are rooted in the generated wrapper.
  Managed return pointers are checked and rooted before the wrapper returns.
  C code that allocates managed objects must use the runtime's root-frame API
  for its own temporary references.
- A missing `<symbol>` is diagnosed by the linker at link time, not by
  semantic analysis. `tests/functional/positive/positive_extern_functions.simp`
  exercises `from` bindings for integer, string argument/return, array, and
  class-reference values; its bundled C shims include a call to libc `abs()`.
- Compiled module imports are implemented using
  `import <module_name> as <symbol>`. The registry defaults to
  `simp-modules.tsv` in the compiler's current working directory; set
  `SIMP_MODULE_REGISTRY` to select another file. Each tab-separated,
  six-field row records module name, Simple source path, version, comma-
  separated `dependency=version` values, export kind (`class` or `namespace`),
  and the one designated top-level export name. Relative source paths are
  relative to the registry file. Module source is compiled to a separate LLVM
  IR unit and linked with the importer; dependencies between Simple modules
  are resolved recursively. Versions are recorded as metadata, not enforced
  constraints, and external binary-library resolution remains deferred.
  Imported declarations are available only through their aliases: a namespace
  alias can qualify nested types (`Net.Http.Client().get("/")`), while a class
  alias is constructed directly (`Client(args).method()`). The import grammar
  and scoping behavior are specified in `SIMPLE-LANGUAGE-NOTES.md`.



LLVM 22.1.8, CMake 3.31.6, GCC 14.2, and Clang 22.1 were available when this
prototype was extended. The implementation emits textual LLVM IR and invokes
Clang; it does not link the LLVM C++ API or provide a configurable LLVM
optimization pipeline. Building the compiler requires Clang on `PATH`; the
current driver launches it through the host POSIX shell. Full language type
checking and name-resolution rules, OOP beyond the supported single- and
multiple-inheritance slices (including access to protected
base members from further-derived classes),
production GC features, external binary-library configuration, inline C, GTK,
package manager, IDE, and debugger remain deferred. Collection deletion and
package resolution remain unimplemented. Namespace, include, and compiled
source-module import behavior is implemented; other design-note proposals may
still be unsupported.
