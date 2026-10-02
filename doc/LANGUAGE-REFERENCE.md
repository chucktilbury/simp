# Simple language reference

Simple is the object-oriented language implemented by this repository's
compiler. This manual describes the working subset: the C++17 compiler
produces textual LLVM IR and invokes Clang to link the runtime. It is not a
description of every feature proposed in the design notes.

Grammar links below name productions in [GRAMMAR.md](GRAMMAR.md). Standard
library package APIs are intentionally covered separately in
[STDLIB.md](STDLIB.md).

## Lexical structure

Identifiers start with an ASCII letter or `_` and continue with letters,
digits, or `_`. Keywords are case-insensitive; identifiers are case-sensitive.
The exact identifier `String` names the built-in string class, while `strg` is
the primitive type keyword for string values. Lowercase `string` is an ordinary
identifier. `list` and `dict` are the collection type keywords; `array` and
`map` are ordinary identifiers. See `IDENT`,
`primitive-type`, and the lexical grammar.

Statements normally end at a newline. Braces delimit blocks; indentation is
for readability only. Newlines inside `()` and `[]` are ignored. `;`, `#`,
and `//` start line comments; `/* ... */` comments can span lines but do not
nestedly pair. A semicolon begins a comment rather than separating statements.
See `NEWLINE`, `COMMENT`, and `terminator`.

Integer literals may be decimal or hexadecimal (`0x`/`0X` followed by
case-insensitive digits `0`-`9` and `a`-`f`). Unsuffixed literals are signed
64-bit `int` values by default; a trailing `u` or `U` selects an unsigned
64-bit literal. The signed range is
-9,223,372,036,854,775,808 through 9,223,372,036,854,775,807; literals outside
the applicable range are rejected. Signs are unary operators, so
`-0x8000000000000000` is the signed minimum, but its positive magnitude is
out of range for `int`. Float literals accept decimal points and exponents.
Double-quoted and single-quoted strings are UTF-8 and cannot span physical
lines. Double quotes support `\e` (ESC, byte `0x1b`), `\n`, `\r`, `\t`,
`\\`, and `\"`; single quotes do not interpret escapes. See `INTEGER`,
`UNSIGNED_INT`, `FLOAT`, `STRING`, and `ESCAPE`.

```simp
// Complete program: literal spellings and a formatted string.
start {
    int count = 0x12
    unsigned total = 0x12u
    float ratio = .5
    print("count={}, total={}, ratio={}"(count, total, ratio))
}
```

## Program structure and entry point

A program has zero or more top-level imports and declarations, followed by
exactly one `start { ... }` block (`program`, `start-block`). The `start`
block is the executable entry point; `return` is reserved for methods and is
not a way to exit from `start`. A separately imported module may contain
declarations but no `start` (`module`). The compiler can also compile several
source files as one compilation unit, with exactly one `start` among them.

```simp
// Complete program: executable statements live in start.
start {
    print("Simple is running")
}
```

## Types and values

The built-in types are:

| Type | Values and behavior |
|---|---|
| `int` | Signed 64-bit integer. |
| `unsigned` | Unsigned 64-bit integer. |
| `float` | IEEE double-precision floating-point value. Literal parsing rejects non-finite/out-of-range values. |
| `bool` | `true` or `false`. Conditions must have this type; there is no general truthiness conversion. |
| `String` / `strg` | Managed UTF-8 string object. `strg` names the existing `String` class type. |
| `list` | Ordered, dynamically sized, heterogeneous collection. |
| `dict` | String-keyed collection with dynamically typed values. |
| `buffer` | Mutable sequence of bytes. |
| `handle` | Opaque native/runtime handle with no built-in language operations. |
| `any` | Internal inferred type for dynamically tagged values, especially collection reads and loop values. It is not a user-declarable type. |
| `type` | Runtime type descriptor, produced by `type(value)` or a type name used as a value. |
| class name | Managed object reference, including `Exception` subclasses. |

Although `any` is a reserved token and an internal inferred type name, users
cannot declare `any` variables, fields, parameters, or return types. Collection
reads and foreach values are inferred as `any` internally; that does not make
`any` legal in a declaration. Such values can be inspected with `is` or
`type(...)`, printed, compared with `null`, and stored or passed where the
dynamic value is supported. To use a value in ordinary member access or
arithmetic, first extract it into a concrete typed local; extraction performs
a runtime type check and raises if the value has a different type. See
`primitive-type`, `type`, `return-type`, and `type-test-name`.

The parser also recognizes `void` as a method return type. It is not a
storable value type. See `primitive-type`, `type`, `return-type`, and
`type-test-name`.

Conversions are deliberately limited. Compatible class references can be
assigned to a base-class variable when the inheritance path is unambiguous
and accessible. Dynamically tagged collection values can be extracted into
supported concrete local types; the runtime checks the contained value.
Explicit numeric casts use
`int(expr)`, `unsigned(expr)`, or `float(expr)`. Implicit numeric conversions
are not general-purpose; overload resolution allows a plain integer literal
(decimal or hexadecimal) to match an `unsigned` parameter, while `u` literals
are unsigned directly.
Integer-to-float casts round to the nearest representable double when needed.
Float-to-integer casts truncate toward zero; NaN and values outside the
target range raise a catchable `integer overflow` exception. Signed
`int` division or remainder with the minimum value and `-1` likewise raises
`integer overflow`.
String-to-number conversion is provided by the instance methods
`value.toInt()`, `value.toUnsigned()`, and `value.toFloat()` and can raise on
invalid input or an integer value outside the target type's range.
See `primary` and `postfix`.

`null` is a universal null value and can initialize nullable locals, including
scalar locals. Such a scalar local carries a null state; reading/casting it as
a scalar when null raises a runtime exception. Null assignment does not make
primitive fields, collection values, arguments, or returns nullable in the
same way: those contexts retain their declared representation. Object and
reference values can be null. Use `is Type` to inspect a value's dynamic type;
`type(value)` returns its type descriptor. See `type-test-name`, `primary`,
and `relational`.

```simp
// Complete program: null locals, casts, and runtime type tests.
start {
    int maybe = null
    if (maybe is int) {
        print(maybe)
    } else {
        print("no integer value")
    }
}
```

## Variables and scope

Declare a local with `type name` and optionally initialize it with
`type name = expression`. Locals are block-scoped and may be initialized
later. An inner declaration shadows an outer binding; assignments to the
inner variable do not change the outer one, and shadowing currently emits no
warning. Redeclaring a name in the same scope is an error. The compiler
diagnoses a read that may occur before initialization. Parameters and fields
are introduced by their declarations; `for` loop variables exist only in the
loop body. Assignment operators are statements, not expressions. `=` assigns
normally; `+=`, `-=`, `*=`, `/=`, and `%=` update an `int`, `unsigned`, or
`float` variable or object field using the corresponding arithmetic operation.
Both operands must have a matching arithmetic type, and `%=` is limited to
`int` and `unsigned`. Collections, buffers, `any`, booleans, and object
references do not support compound assignment. A compound assignment evaluates
its target once; for a member target, its receiver is evaluated once. Integer
overflow and integer division/remainder errors follow the corresponding
arithmetic operators.

```simp
// Complete program: an inner value shadows, rather than changes, the outer value.
start {
    int value = 40
    {
        int value = 2
        value = value + 1
        print(value)
    }
    print(value)
}
```

This prints `3`, then `40`, on separate lines.

## Expressions and operators

Expressions include literals, names, parentheses, list and dict literals,
casts, `type(value)`, `buffer(length)`, class construction, method calls,
member access, indexing, slicing, and type tests. Calls are written as
`ClassName(arguments)` for construction or `object.method(arguments)` for
methods. The parser does not support free functions, closures, or arbitrary
callable-valued expressions. See `expression`, `primary`, `postfix`, and
`constructor-declaration`.

Operators from lowest to highest precedence:

| Operators | Associativity |
|---|---|
| `or`, `\|\|` | left |
| `and`, `&&` | left |
| `not`, `!` | right prefix |
| `==`, `!=` | left |
| `is`, `<`, `<=`, `>`, `>=` | left |
| `+`, `-` | left |
| `*`, `/`, `%` | left |
| unary `+`, unary `-` | right prefix |
| `.`, calls, `[]` indexing/slicing | repeated postfix |

Logical operators require booleans. Arithmetic and comparison support depend
on operand types; unsupported combinations are rejected during semantic
analysis. Assignment is not an expression. There are no bitwise, increment,
or ternary operators.

Double-quoted text can be formatted by calling the literal with positional
arguments, such as `"value: {}"(value)`, or with named arguments, such as
`"user {name} has {count} messages"(count=total, name=user)`. Named arguments
bind case-sensitively to placeholder names regardless of argument order.
Each distinct placeholder needs exactly one named argument; a name may be
referenced repeatedly, and its argument expression is evaluated once.
Positional `{}` and named `{name}` forms cannot be mixed in one format call.
Use `{{` and `}}` for literal braces. Formatting is not automatic
interpolation. Arguments support the same printable scalar, string, type, and
dynamic values as positional formatting. `print` takes at most one
expression; it prints simple scalar/string/type values, or a double-quoted
format call. See `format-suffix` and `print-statement`.

```simp
// Complete program: arithmetic, logical operators, and formatting.
start {
    int first = 20
    int second = 22
    bool ready = first > 0 and second > 0
    if (ready) {
        print("answer={}"(first + second))
    }
}
```

## Statements

The supported statements are local declarations, assignment, method-call
expression statements, `print`, nested blocks, `if`/`else`, `while`,
`do`/`while`, `for`-each, `break`, `continue`, `return`, `try`/`except`/
`finally`, `raise`, base-constructor initializers, and inline C. See
`statement` and its named productions.

Conditions are parenthesized and must be `bool`. `else` always introduces a
block; there is no `else if` production. The `do` body executes at least once.
`for (value in collection)` iterates list values; `for (key, value in dict)`
iterates dict keys and values. The value variable has type `any`; the key
variable has type `String`. `break` and `continue` are valid only in loops.

```simp
// Complete program: while, continue, and for-each.
start {
    list values = [2, 3, 4]
    int total = 0
    for (value in values) {
        if (value is int) {
            int number = value
            if (number == 3) {
                continue
            }
            total = total + number
        }
    }
    print(total)
}
```

## Classes, objects, inheritance, and dispatch

Classes contain fields and methods. Member access is `public` by default;
`private:` and `protected:` switch the access level for following members;
`public:` restores public access.
Fields have no in-class initializer syntax. A constructor is named after its
class and can be overloaded. `destroy()` is the destructor form. Methods can
be defined in the class body or declared there and defined out of line. See
`class-declaration`, `class-member`, `constructor-declaration`,
`destructor-declaration`, `method-declaration`, and
`out-of-line-definition`.

Inheritance uses a colon and one or more comma-separated base specifiers.
Each base defaults to public access. `public`, `protected`, or `private`
controls the inheritance path; `virtual` may also mark a base. Multiple
inheritance, virtual bases, and base-qualified member access are implemented.
Ambiguous inherited fields/methods must be qualified through a base.
Constructors initialize direct bases with `super Base(args)` as leading
statements. Virtual bases use either `super virtual Base(args)` or
`virtual super Base(args)`; the two forms have identical semantics. A dot is
not allowed between these keywords or the base name. A derived class may
override an inherited method with a compatible signature. Calls use the
runtime object's method dispatch; there is no separate `virtual` method
modifier.

```simp
// Complete program: inheritance, construction, override, and dispatch.
class Meter {
    int value
    Meter(int initial) {
        value = initial
    }
    int read() {
        return value
    }
}

class AdjustedMeter : public Meter {
    AdjustedMeter(int initial) {
        super Meter(initial)
    }
    int read() {
        return value + 1
    }
}

start {
    Meter meter = AdjustedMeter(41)
    print(meter.read())
}
```

The above is a complete program; base-qualified member access uses an object
and a base name (for example, `object.Base.method()`), while a `super` base
initializer is only for constructor initialization. The base initializer must
appear before ordinary constructor statements.

Explicit `object.destroy()` invokes the destructor but does not reclaim the
object; the compiler emits a warning. Destructors cannot be defined out of
line. See `super-initializer` and `postfix`.

## Namespaces

Namespaces group classes and can nest or reopen. A class's namespace prefix
is used when resolving its type and base names. `start`, `import`, and
`include` cannot be declared within a namespace. Out-of-line definitions must
be in the same namespace scope as the class they define. See
`namespace-declaration` and `namespace-item`.

```simp
// Complete program: a class in a namespace.
namespace Geometry {
    class Point {
        int x
        Point(int value) {
            x = value
        }
        int coordinate() {
            return x
        }
    }
}

start {
    Geometry.Point point = Geometry.Point(7)
    print(point.coordinate())
}
```

## Imports, packages, and textual inclusion

`import Name as Alias` resolves a module or package through the module
registry and makes its exported namespace itself available through the alias:
the alias replaces the package's namespace prefix. For example,
`import time as T` exposes `Clock` as `T.Clock`, not `T.Time.Clock`.
Standard-library classes are instance-based: construct the class before
calling an instance method, as in `Sys.Process().exit(0)`, rather than using
a static-style call such as `Sys.Process.exit(0)`.
Imports are top-level only and imported module source cannot define `start`.
Package manifests and package APIs are documented in [STDLIB.md](STDLIB.md).
See `import-declaration` and `module`.

`include "relative/path.simp"` textually inserts source before parsing. It is
only allowed before the top-level `start`; the including file's directory is
searched first, followed by configured include paths. Each canonical file is
included once, and included files cannot define `start`. Unlike imports,
includes do not create an isolated module namespace. See
`include-directive` and the source-loader notes in [GRAMMAR.md](GRAMMAR.md).

```simp
// Complete program (requires an installed/bundled `system` package).
import system as Sys
start {
    Sys.Process().exit(0)
}
```

```simp
// Fragment (include is expanded before parsing):
include "shared/definitions.simp"
start {
    print("included source participates in this compilation unit")
}
```

## Exceptions

`Exception` is a built-in base class with a public `message` field.
`raise(ClassName(arguments))` raises an instance of `Exception` or a subclass.
Handlers use `except(Type)` for a typed catch, `except() as name` for a
catch-all whose binding is a `String` message, or `except(Type) as name` for
an exception-object binding. A bare `raise()` rethrows the active exception
and is valid only inside a handler. `finally` runs for normal completion and
exception propagation. A `try` needs at least one `except` or a
`finally`; a catch-all must be last. Method returns must be the final direct
statement in a method body; a return cannot be nested inside
`try`/`except`/`finally`. See `try-statement` and `raise-statement`.

```simp
// Complete program: raising and catching a typed exception.
class Problem : public Exception {
    Problem(String text) {
        super Exception(text)
    }
}

start {
    try {
        raise(Problem("broken"))
    } except(Problem) as problem {
        print(problem.message)
    } finally {
        print("finished")
    }
}
```

## Strings and their methods

String literals construct managed `String` values. Strings expose a
read-only `length` member (the UTF-8 byte length) and methods for appending,
equality, numeric conversion, byte access, slicing, insertion/removal, search,
splitting, replacement, trimming, and case conversion. `toUpper()` and
`toLower()` perform ASCII case conversion; `trim()`/`strip()` trim ASCII
whitespace. See the `String` section of
[STDLIB.md](STDLIB.md) for the exact standard API. String operations use
UTF-8; byte offsets and character boundaries are not interchangeable.
Out-of-range indices and invalid conversions can raise exceptions. See
`STRING` and `postfix` for literal and call syntax.

```simp
// Complete program: basic string operations.
start {
    String text = "Hello"
    text.append(" world")
    print(text)
    print(text.length)
}
```

## `buffer` and `handle`

`buffer(length)` creates a mutable byte buffer. It has a read-only `length`,
supports integer indexing and slicing, and provides `resize`, `clear`, and
`append(int-or-unsigned)` methods. Indexed values are `unsigned`; indexed
assignment accepts `int` or `unsigned`. Bounds and allocation failures are
checked by the runtime.

`handle` is an opaque, nullable native/runtime handle. It has no language
operators, members, or automatic ownership policy; a library binding must
define how a handle is created and released. `buffer` and `handle` values can
be stored in collections and passed through `any`. See `primary`,
`index-or-slice-suffix`, and `postfix`.

```simp
// Complete program: byte-buffer access and mutation.
start {
    buffer bytes = buffer(2)
    bytes[0] = 65
    bytes.append(66)
    print(bytes.length)
    print(bytes[0])
}
```

```simp
// Fragment: native handles are opaque and must be released by their owner.
handle nativeResource = null
```

## Collections

List literals use `[...]` and may mix supported scalar values, strings,
class references, lists, dicts, buffers, handles, and `null`. Values are
stored with dynamic tags, but `any` is not a type that can be declared for an
element, variable, field, parameter, or return.
`list.length` is read-only; `append(value)` and `resize(int)` mutate it.
Although literals accept buffers and handles, the current `append` semantic
check does not accept those two element types directly. There is no list
element deletion operation; dicts separately provide `remove(string)`.
Indexing and foreach iteration produce values inferred as `any` internally,
not declared `any` variables. A dynamic value can be type-tested, passed,
printed, or compared with `null`; extract it into a concrete local before
using ordinary member access or arithmetic. The runtime checks extraction.
List slices return a copy and can include a step:
`values[start:end:step]`. Bounds are checked by the runtime.

Dict literals use `{ key: value, ... }`. Keys must be strings; values may be
supported scalar/reference/collection types. Indexed reads and foreach
iteration produce dynamically tagged values inferred internally as `any`,
not legal `any` declarations. Dicts
support `length`, `contains(string)`, `remove(string)`, string-key indexing,
and slicing. Dict slice bounds are integer positions in iteration order;
stepped dict slices are not supported.

In the current runtime implementation, dict keys are hashed bytewise using
64-bit FNV-1a (offset basis `14695981039346656037`, prime `1099511628211`).
The hash index uses a power-of-two, open-addressed bucket table with linear
probing; the table grows and is rebuilt to keep its load at or below one
half. After a hash match, key byte length and byte contents are compared.
Entries themselves are stored in insertion order, so current iteration and
dict slicing follow insertion order. These are implementation details, not a
stable complexity guarantee. See `list-literal`, `dict-literal`,
`index-or-slice-suffix`, `foreach-statement`, and [STDLIB.md](STDLIB.md).

```simp
// Complete program: type-check and extract an indexed collection value.
start {
    list values = [42, "text"]
    if (values[0] is int) {
        int number = values[0]
        print(number)
    }
}
```

## Threads and concurrency semantics

Thread support is provided through classes and native bindings, not a
threading keyword. The bundled runtime starts OS threads for thread objects
and dispatches their `run()` method. Simple-generated code executes under one
global runtime lock, so two Simple bodies do not execute managed code in
parallel. Blocking native operations such as `join()` and semaphore waits
release that lock, allowing another Simple thread to run. This provides
concurrency around waits, not parallel execution of Simple instructions.
The relevant source forms are `class-declaration`, `method-declaration`, and
`postfix`; none is a dedicated thread construct.

Use the standard synchronization package for its supported thread,
semaphore, mutex, and condition facilities; see [STDLIB.md](STDLIB.md).
Native handles and synchronization resources require the library's documented
lifetime operations. Guard shared state with the appropriate synchronization
primitive even though execution is serialized: native calls can block and
release the global lock.

```simp
// Fragment: requires a visible `Thread` base class supplied by declarations
// or a package. Thread operations are native bindings, not built-in syntax.
class Worker : Thread {
    void run() {
        print("worker")
    }
}
```

## Compiler and executable backend

The compiler lexes and parses Simple source, performs semantic analysis, emits
textual LLVM IR, and invokes Clang to compile and link the executable. Use
`--check-only` to run parsing and semantic checks without code generation, or
`--emit-llvm FILE` to also save the generated IR. The full command-line
interface is documented in [simp(1)](simp.1).
The compiler does not link the LLVM C++ API or provide a configurable LLVM
optimization pipeline; its Clang invocation is driven through the host POSIX
shell.

The implemented backend covers the language constructs in this reference:
scalar and String values, collections with tagged dynamic values, control
flow, exceptions, class construction and methods, inheritance (including
secondary and virtual bases), virtual dispatch, and native/inline-C bindings.
Printing accepts a single supported value or a double-quoted positional or
named format call. For how the source, parser, runtime, and tests are
organized, see [SIMPLE-LANGUAGE-NOTES.md](SIMPLE-LANGUAGE-NOTES.md) and the
[test guide](../tests/README.md).

Managed objects and collection values use a precise, non-moving,
stop-the-world mark/sweep collector. Generated functions publish explicit
root frames; arbitrary native stack words are not scanned. The runtime
serializes Simple execution under a global lock. OS threads can make progress
while another thread waits in a blocking native operation, but Simple
instructions do not execute in parallel. Exceptions use the host C ABI's
`setjmp`/`longjmp` mechanism rather than LLVM landing pads.

Known language/backend boundaries include no free-standing functions, no
automatic user-object `toString()` dispatch, no direct String index/slice
syntax, no Unicode code-point operations, and no native-ABI lowering for
`any`. There is no general reflection API beyond `type(value)`. String
methods, Simple class methods, and C-bound methods described elsewhere remain
available; these limitations should not be confused with the obsolete claim
that formatted expressions or method overloading are unsupported.

## Garbage collection and destruction

Managed objects, strings, and collections are allocated on a runtime-managed
heap. The collector traces live roots (including locals and collection
references) and reclaims unreachable objects. A class `destroy()` method is
used as a finalizer when the collector reclaims an instance; collection timing
is not deterministic, so finalizers should not be used as a substitute for
explicit resource management. Finalizers must not let exceptions escape.
Explicit `object.destroy()` runs the method but does not free the object, and
the compiler warns about that distinction.

Derived and base destruction is handled by runtime class finalization; do not
assume a particular collection point or use-after-finalization behavior.
Resources requiring prompt release should be closed explicitly through their
library API. See `destructor-declaration` and `postfix`, and
[STDLIB.md](STDLIB.md).

```simp
// Fragment: finalizer timing is controlled by garbage collection.
class Resource {
    void destroy() {
        print("resource became unreachable")
    }
}
```

## Native bindings for library authors

Declare a method in a class and define it outside the class with a `from`
binding:

```simp
// Fragment: declaration and matching external C ABI binding.
class System {
    String lastError()
}

String System.lastError() from "simp_system_last_error"
```

The external symbol must be linkable by Clang and use the runtime's expected
ABI. Primitive parameters/results use the corresponding C/LLVM scalar
representation (`int` is signed 64-bit, `bool` is 1-bit in IR, `float` is double,
`unsigned` is 64-bit); strings, lists, dicts, buffers, handles, and class
references are pointer-based. `void` is allowed only as a method result.
Declarations and definitions must have matching names, parameter types, and
return types, and native definitions must be in the same compilation unit
and namespace as their class. See `method-declaration` and
`out-of-line-definition`.

`inline { ... }` embeds raw C in a method or start block; optional
`(type name, ...)` captures expose existing Simple locals to the generated
code. The C fragment is passed to the generated LLVM/C ABI boundary and must
respect the runtime's rooting and value representation. Inline C and native
bindings are low-level interfaces; prefer package-level wrappers for public
APIs. See `inline-c-statement` and `capture-list`.

```simp
// Fragment: the declared local is implicitly captured by this inline C block.
int result inline {
    *result = 42;
}
```
