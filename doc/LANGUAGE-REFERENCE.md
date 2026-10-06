# Simple language reference

Simple is the object-oriented language implemented by this repository's
compiler. This manual describes the working subset: the C++17 compiler
produces textual LLVM IR and invokes Clang to link the runtime. It is not a
description of every feature proposed in the design notes.

The parser limits recursive syntax nesting to 128 active levels, counting
namespaces, statement blocks, expressions, and unary operators. Excessive
nesting is a compile-time diagnostic, not a process crash. This protects the
recursive parser from malformed or adversarial inputs; ordinary programs
remain well below the limit.
Expression trees are also limited to depth 128, including flat operator/member
chains, to protect later recursive analysis and AST cleanup. Long shallow
collections are not restricted by this depth limit.

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
// test: {"stdout": "count=18, total=18, ratio=0.5"}
start {
    int count = 0x12
    unsigned total = 0x12u
    float ratio = .5
    print(format("count={}, total={}, ratio={}", count, total, ratio))
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
// test: {"stdout": "Simple is running"}
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
Explicit scalar casts use
`bool(expr)`, `int(expr)`, `unsigned(expr)`, or `float(expr)`. All sixteen
ordered pairs of these scalar types are supported, including identity casts.
The operand must have one of these four static types; class, reference,
string, dynamically tagged `any`, and bare `null` operands are not supported.
Implicit numeric conversions
are not general-purpose; overload resolution allows a plain integer literal
(decimal or hexadecimal) to match an `unsigned` parameter, while `u` literals
are unsigned directly.
The following table is normative (rows are source types, columns are targets):

| Source / target | `bool` | `int` (signed i64) | `unsigned` (u64) | `float` (double) |
|---|---|---|---|---|
| `bool` | Identity | `false` = 0, `true` = 1 | `false` = 0, `true` = 1 | `false` = 0.0, `true` = 1.0 |
| `int` | Zero = false, nonzero = true | Identity | Value modulo 2^64 (same bits) | IEEE double conversion |
| `unsigned` | Zero = false, nonzero = true | Checked value <= 9223372036854775807 | Identity | IEEE double conversion |
| `float` | Zero = false, nonzero (including NaN) = true | Checked, truncate toward zero | Checked, truncate toward zero | Identity |

Integer-to-float conversion rounds to the nearest representable double when
needed. Casting `int` to `unsigned` preserves the 64-bit pattern and yields
the value modulo 2^64, without overflow: `unsigned(-10)` is
18446744073709551606, `unsigned(-1)` is 18446744073709551615, and
`unsigned(-9223372036854775808)` is 9223372036854775808. Printing these
results uses unsigned decimal notation. The reverse cast remains checked:
an `unsigned` greater than 9223372036854775807 cast to `int` raises a
catchable `integer overflow` exception, rather than reinterpreting the bits.
Float-to-integer casts check the input **before** truncating toward zero.
For `int`, the accepted interval is [-9223372036854775808, 9223372036854775808);
for `unsigned`, it is [0, 18446744073709551616). NaN, either infinity, and
values outside these intervals raise the same exception, including negative
fractions or negative subnormals cast to `unsigned`.
For `bool(floatValue)`, both +0.0 and -0.0 are false; every other value,
including positive/negative subnormals, infinities, and NaN, is true.
Identity casts preserve the scalar payload, including floating-point signed
zero and NaN. Signed
`int` division or remainder with the minimum value and `-1` likewise raises
`integer overflow`.
String-to-number conversion is provided by the instance methods
`value.toInt()`, `value.toUnsigned()`, and `value.toFloat()` and can raise on
invalid input or an integer value outside the target type's range.
See `primary` and `postfix`.

`null` is a universal null value and can initialize nullable locals, including
scalar locals. Such a scalar local carries a null state for assignment,
printing, and comparison with `null`. An explicit cast consumes that state:
it converts the local's zero payload and returns a non-null scalar, even for
an identity cast. Thus `bool(nullFloat)` is false, `float(nullInt)` is 0.0,
and `int(nullInt)` is a non-null 0; none raises an exception.
Null assignment does not make
primitive fields, collection values, arguments, or returns nullable in the
same way: those contexts retain their declared representation. Object and
reference values can be null. Use `is Type` to inspect a value's dynamic type;
`type(value)` returns its type descriptor. See `type-test-name`, `primary`,
and `relational`.

```simp
// Complete program: null locals, casts, and runtime type tests.
// test: {"stdout": "no integer value"}
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

Declare a local with `type name`. Class-typed locals may be constructed
directly with `ClassName name(arguments)`, including `ClassName name()` for
the zero-argument constructor. This form selects the matching constructor
using the usual overload, type, and access checks. Built-in types do not use
this syntax. The existing `type name = expression` form remains supported,
including constructor expressions such as `Base item = Derived()`; constructor
expressions also remain valid in returns, call arguments, and other expression
contexts. Locals are block-scoped and may be initialized later. An inner
declaration shadows an outer binding; assignments to the inner variable do not
change the outer one, and shadowing currently emits no warning. Redeclaring a
name in the same scope is an error. The compiler diagnoses a read that may
occur before initialization. Parameters and fields are introduced by their
declarations; fields have no initializer syntax, and `for` loop variables
exist only in the loop body. Assignment operators are statements, not
expressions. `=` assigns normally; `+=`, `-=`, `*=`, `/=`, and `%=` update an
`int`, `unsigned`, or `float` variable or object field using the corresponding
arithmetic operation.
Both operands must have a matching arithmetic type, and `%=` is limited to
`int` and `unsigned`. Collections, buffers, `any`, booleans, and object
references do not support compound assignment. A compound assignment evaluates
its target once; for a member target, its receiver is evaluated once. Integer
overflow and integer division/remainder errors follow the corresponding
arithmetic operators.

```simp
// Complete program: an inner value shadows, rather than changes, the outer value.
// test: {"stdout": "340"}
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

This writes `340`; `print` does not add line breaks.

## Expressions and operators

Expressions include literals, names, parentheses, list, dict, and buffer
literals, casts, `type(value)`, `buffer(length)`, class construction, method calls,
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
| `.`, calls, `[]` indexing/slicing, `as` checked extraction | repeated postfix |

Logical operators require booleans. Arithmetic and comparison support depend
on operand types; unsupported combinations are rejected during semantic
analysis. Assignment is not an expression. There are no bitwise, increment,
or ternary operators.

### Checked extraction with `as`

`value as Target` is checked extraction/identity, not a numeric conversion.
Targets are `int`, `unsigned`, `bool`, `float`, `strg` (also spelled `String`),
`list`, `dict`, `buffer`, `handle`, `type`, `null`, `any`, or a declared class
(including a qualified class name). `void` and unknown names are compile-time
errors. The source is evaluated exactly once.

An internal `any` source, such as a list or dict read or foreach value, is
checked against the requested dynamic tag. Scalars and type descriptors require
the exact matching tag; `strg`/`String` requires a String object; list, dict,
buffer, and handle targets require their respective reference tags; class
targets accept the exact class or a subclass.
Class extraction adjusts to the requested unique base subobject, including
secondary and virtual bases. Ambiguous repeated nonvirtual bases and all
mismatches raise catchable, source-located runtime exceptions. Use `is` to
inspect mixed bags without raising on a mismatch.

Statically known sources may be extracted identically; class references may
also be checked against another class target. Other statically impossible
source/target pairs are rejected during semantic analysis. `as any` boxes a
statically typed supported value, or preserves an existing `any`; the `null` literal boxes as the dynamic null value. `as null` checks
for a null pointer in a reference payload (class/String, list, dict, buffer,
or handle) or a dynamic null. It does not recognize null-valued scalar slots
or type descriptors.

Null extraction follows the existing typed-extraction representation:
Class/String extraction preserves the canonical dynamic null payload.
`buffer` extraction accepts that canonical null tag; `handle` extraction
accepts it and also accepts a null pointer carrying the handle tag. `list` and
`dict` extraction require their matching tag and a non-null collection
reference. Scalar and `type` targets require their exact tags and therefore
reject null. Statically typed same-type expressions remain identity
operations and preserve their existing nullable value. No new
nullable-scalar behavior is introduced.

Use `(items[index] as Foo).method()` for member calls without a temporary
local. Qualified targets work, for example
`(mapping["item"] as model.Foo).method()`. Casts bind at postfix precedence.
`Foo(value)` remains construction, never a cast, and scalar conversions retain
their existing `int(value)`, `bool(value)`, `unsigned(value)`, and
`float(value)` syntax.

Literal text can be formatted with the `format` intrinsic and positional
arguments, such as `format("value: {}", value)`, or with named arguments, such as
`format("user {name} has {count} messages", count=total, name=user)`. Named arguments
bind case-sensitively to placeholder names regardless of argument order.
Each distinct placeholder needs exactly one named argument; a name may be
referenced repeatedly, and its argument expression is evaluated once.
Positional `{}` and named `{name}` forms cannot be mixed in one format call.
Use `{{` and `}}` for literal braces. Formatting is not automatic
interpolation. Arguments support the same printable scalar, string, type, and
dynamic values as positional formatting. `format` returns a fresh managed `strg`,
usable anywhere an expression is accepted. Templates must be string literals
(either quote style); dynamic templates are not supported. Arguments are
evaluated and snapshotted once, left-to-right, before assembling the result.
The old string-literal-call syntax is rejected, not an alias.

`print(value)` retains scalar/string/type/dynamic printing. `print("literal")`
prints the literal unchanged, including braces. `print("value: {}", value)`
formats with the same rules as `format`, then writes the completed result
without adding a newline; formatting failure emits no partial output.

An optional `:` introduces this deliberately limited, C++20-inspired subset,
not full `std::format` compatibility:

| Specifier | Meaning |
|---|---|
| `d` | Decimal `int` or `unsigned` |
| `x`, `X` | Lower-/upper-case hexadecimal, without prefix; negative signed values use sign plus magnitude |
| `c` | One ASCII byte from an integral value in 0..127, including NUL |
| Width | Minimum field width, at most 1000000; never truncates |
| `<`, `>`, `^` before width | Left, right, or center space padding; an odd center-padding remainder goes on the right |
| Leading `0` before width | Numeric zero padding after the minus sign; integral values only; incompatible with explicit alignment and `c` |

The order is `[alignment][width][type]`, for example `{:x}`, `{:08X}`,
`{:c}`, `{:>8}`, or `{value:08X}`. Empty specs retain default rendering.
Default alignment is right for numeric values and left otherwise. Width counts
Unicode code points, not bytes or terminal display columns. Floating-point
precision, custom fill characters, positional indices, and other type codes
are unsupported. Explicit type codes require `int`/`unsigned`; statically
known incompatible types and malformed templates are compile-time errors.
Dynamic type mismatches and ASCII range violations raise catchable exceptions
located at the argument expression. Negative values and 128+ never generate
invalid UTF-8. See `format-expression` and `print-statement` in the grammar.

```simp
// Complete program: arithmetic, logical operators, and formatting.
// test: {"stdout": "answer=42"}
start {
    int first = 20
    int second = 22
    bool ready = first > 0 and second > 0
    if (ready) {
        print(format("answer={}", first + second))
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
// test: {"stdout": "6"}
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
class, omits the return type (write `Name(...)`, not `void Name(...)`), and
can be overloaded. A class-body method named after its class with any return
type is rejected rather than treated as an ordinary method. A destructor is written `destroy { ... }` inside
the class; it has no return-type or parameter list. Methods can be defined in
the class body or declared there and defined out of line. See
`class-declaration`, `class-member`, `constructor-declaration`,
`destructor-declaration`, `method-declaration`, and
`out-of-line-definition`.

An anonymous class-scoped enum block declares immutable signed 64-bit `int`
constants, not an enum type:

```simp
class Status {
    // test: {"stdout": "12"}
    public:
    enum {
        READY = 10,
        RUNNING,
        COMPLETE = READY + 2,
    }
}

start {
    Status status = Status()
    int code = status.RUNNING
    print(Status.COMPLETE)
}
```

The first omitted value in each block is `0`; each later omitted value is the
preceding value plus one. Explicit values may use integer literals, unary
`+`/`-`, `+`, `-`, `*`, `/`, `%`, and previously resolved enum members. Values
are checked for signed 64-bit overflow, including implicit increments;
duplicate numeric values are allowed, but duplicate names and class-member
collisions are errors. Forward/cyclic references, nonconstant expressions,
division by zero, and out-of-range values are compile-time errors. Multiple
blocks may appear in one class, with distinct member names.

Constants are usable anywhere an `int` expression is accepted, including
method arguments, collection values, and scalar casts. They follow the
declaring member's public/protected/private access and ordinary inherited
member lookup; ambiguous inherited constants need explicit base qualification.
Use either `object.NAME` or `Class.NAME`, including namespace-qualified class
names. Instance-qualified access resolves from the static class and does not
dereference or null-check the receiver, but a receiver expression with side
effects is still evaluated exactly once. Constants have no runtime allocation
or per-instance storage. Assignment, compound assignment, and inline field
captures are rejected; copy the constant into a local before modifying or
capturing it.

Inheritance uses a colon and one or more comma-separated base specifiers.
Each base defaults to public access. `public`, `protected`, or `private`
controls the inheritance path; `virtual` may also mark a base. Multiple
inheritance, virtual bases, and base-qualified member access are implemented.
Ambiguous inherited fields/methods must be qualified through a base.
Constructors initialize direct bases with `super Base(args)` as leading
statements. Virtual bases use either `super virtual Base(args)` or
`virtual super Base(args)`; the two forms have identical semantics. A dot is
not allowed between these keywords or the base name. Virtual inheritance is
distinct from virtual method dispatch: it shares a base subobject across
inheritance paths, while virtual method dispatch selects an override at
runtime. A non-virtual base is a separate subobject on each path.

Only the most-derived constructor initializes the complete object's virtual
bases. Every class that has virtual bases may declare `super virtual Base(...)`,
even when it is also used as a base class, and may be constructed directly.
When that constructor runs for a base subobject, its virtual-base initializers
are skipped and their argument expressions are not evaluated. The
most-derived constructor initializes each virtual base once; if it omits an
initializer, that base's zero-argument constructor runs automatically. If the
base has no zero-argument constructor, constructing the most-derived class
without an explicit initializer is a compile-time error. Virtual bases are
constructed before direct non-virtual bases in depth-first, left-to-right
virtual-base order, with virtual ancestors before their descendants. A shared
virtual base is destroyed once, after non-virtual bases, in reverse virtual
construction order; repeated non-virtual base subobjects are each destroyed
separately.

A derived class may override an inherited method with a compatible signature.
Calls use the runtime object's method dispatch; there is no separate `virtual`
method modifier.

A constructor may instead begin with a single `try` whose body consists only
of its base initializers. Required direct bases must still be initialized
exactly once in declared order; explicit virtual initializers precede them
in virtual-base construction order and only initialize virtual bases when this
constructor is running for the complete object.
Automatic default virtual-base initialization also runs inside this `try`.
Every `except` handler must raise or rethrow on every path (nested blocks,
`if`/`else`, and `try` are supported); any `return`, including a nested or
unreachable return, is rejected. A loop alone is not proof of exceptional
termination. Optional `finally` uses the normal exception cleanup rules.
Handlers cannot recover a failed initialization: the constructor cannot
continue or return a successfully constructed partial object. No destructor
or finalizer runs for the failed allocation. `raise()` retains the original
exception object, raise location, and stack trace; a replacement `raise`
creates a new exception. Initializers in handlers, cleanup, branches, loops,
later statements, or mixed direct/protected sequences are rejected.

```simp
class Foo {
    // test: {"stdout": "caught the first timethis is the stringcaught the second time"}
    Foo() { raise(Exception("this is the string")) }
}
class Bar : Foo {
    Bar() {
        try { super Foo() }
        except() {
            print("caught the first time")
            raise()
        }
    }
}
start {
    try { Bar b() }
    except() as e {
        print(e)
        print("caught the second time")
    }
}
```

```simp
// Complete program: inheritance, construction, override, and dispatch.
// test: {"stdout": "42"}
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

The above is a complete program. Base-qualified member access uses an object
and a base path (for example, `object.Base.field`, `object.Base.method()`, or
`object.Left.Root.field`). Inside a method, constructor, or destructor body,
the receiver may be omitted: `Base.field` and `Base.method(args)` refer to the
current object's base subobject, not static or class-level storage.
`Left.Root.field` and `Left.Root.method(args)` select successive direct bases;
the path cannot skip an inheritance edge. Reads, assignments, and supported
scalar compound assignments (`+=`, `-=`, `*=`, `/=`, `%=`) use the same base
layout and accessibility rules as explicit-object access. Method calls retain
runtime dispatch, just like `object.Base.method()`; qualification selects a
base view, not a non-virtual call.

A local variable or parameter takes precedence over a field, and a field takes
precedence over an implicit base qualifier, even if its name matches a base.
Thus `Base.field` uses an ordinary object receiver when `Base` names a variable
or field; it does not fall back to base qualification if that receiver is
invalid or ambiguous. Otherwise, the initial class name is resolved using the
normal lexical namespace/import-alias rules and must name a direct base.
Both `Base.field` and `Namespace.Base.field` can supply that initial class name.
Subsequent base steps follow the existing explicit-object base-path rules.
Type-valued contexts and qualified class construction retain their existing
name resolution; `Namespace.Class()` still constructs an object when its
receiver prefix is not an applicable implicit base.

Private inheritance permits access from the class declaring that inheritance
but not from outside it or further derived classes. Base-private members
remain inaccessible; protected members follow the ordinary derived-class
access rules. Ambiguous unqualified members remain errors. This syntax does
not enable `Base.field` or `Base.method()` outside an applicable class body.

A `super` base initializer is only for constructor initialization and must
appear before ordinary constructor statements. Field visibility uses sections
such as `public:` followed by separate field declarations, not inline
`public int field` declarations.

Explicit `object.destroy()` invokes the destructor but does not immediately
reclaim the object's storage, and no compiler warning is emitted. The call
still uses parentheses; only the in-class definition omits them. The object
is marked destroyed before the destructor runs; a repeated destruction call
or later use of the object raises a catchable runtime exception. Destructors
cannot be defined out of line. See `super-initializer` and `postfix`.

## Namespaces

Namespaces group classes and can nest or reopen. A class's namespace prefix
is used when resolving its type and base names. `start`, `import`, and
`include` cannot be declared within a namespace. Out-of-line definitions must
be in the same namespace scope as the class they define. See
`namespace-declaration` and `namespace-item`.

```simp
// Complete program: a class in a namespace.
// test: {"stdout": "7"}
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
    Geometry.Point point(7)
    print(point.coordinate())
}
```

## Imports, packages, and textual inclusion

`import Name` resolves a package and exposes its declared exported namespace
(or exported class) under the export's own name. For example, `import system`
exposes `System.Process`, `System.File`, and `System.System`; it does not guess
a namespace from the lowercase package identifier. `import Name as Alias`
makes that same export available through the alias:
the alias replaces the package's namespace prefix. For example,
`import time as T` exposes `Clock` as `T.Clock`, not `T.Time.Clock`.
Standard-library classes are instance-based: construct the class before
calling an instance method, as in `Sys.Process().exit(0)`, rather than using
a static-style call such as `Sys.Process.exit(0)`.
Imports are top-level only and imported module source cannot define `start`.
Duplicate bindings, conflicts with local declarations, and namespaces shared
by different packages are diagnosed; use explicit aliases to choose local
binding names (aliases do not make conflicting package definitions compatible).
The `String` prelude remains implicit and does not need an import.
Package manifests are documented in [PACKAGES.md](PACKAGES.md) and package
APIs in [STDLIB.md](STDLIB.md).

New projects use the direct dependency manifest `simpkg.toml` and the generated
exact graph `simpkg.lock`. `simpkg add OWNER/REPO --yes` resolves and installs
the full graph; compilation then needs no environment activation and never
fetches code. Compilation validates lock freshness, exact dependencies, and
package integrity. The lock is discovered from the source project's root and
cannot be relocated by package storage overrides. See
[project module search](INSTALLATION.md#project-module-search) for the ordered
CLI, project, environment, user, and installation package roots. The legacy
`modules.toml`, tab-separated registry, `--package-path`, and
`SIMP_PACKAGE_PATH` lookup mechanisms are no longer supported.
See `import-declaration` and `module`.

`include "relative/path.simp"` textually inserts source before parsing. It is
only allowed before the top-level `start`; the including file's directory is
searched first, followed by configured include paths. Each canonical file is
included once, and included files cannot define `start`. Unlike imports,
includes do not create an isolated module namespace. See
`include-directive` and the source-loader notes in [GRAMMAR.md](GRAMMAR.md).

```simp
// Complete program (requires an installed/bundled `system` package).
// test: {"stdout": ""}
import system
start {
    System.Process().exit(0)
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
`finally`; a catch-all must be last. A method may return from any executable
statement, including nested blocks, branches, loops, and exception handlers.
Returning runs enclosing `finally` blocks from innermost to outermost before
leaving the method. A return inside `finally` (even in a nested statement)
is invalid. A non-void method must return a value or raise on every reachable
path; loops alone do not guarantee this. See `try-statement` and
`raise-statement`. Constructors and destructors remain void-only, and `start`
still cannot return.

```simp
// Complete program: raising and catching a typed exception.
// test: {"stdout": "brokenfinished"}
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
// test: {"stdout": "Hello world11"}
start {
    String text = "Hello"
    text.append(" world")
    print(text)
    print(text.length)
}
```

## `buffer` and `handle`

`buffer(length)` creates a mutable byte buffer of `length` zero-filled bytes.
`buffer[byte, ...]` creates a new buffer initialized from its elements;
`buffer[]` creates an empty buffer. Each element must have type `int` or
`unsigned`, is evaluated once from left to right, and contributes its low 8
bits, matching indexed writes and `append`. Bare `[ ... ]` remains a
heterogeneous list literal. A buffer has a read-only `length`, supports
integer indexing and slicing, and provides `resize`, `clear`, and
`append(int-or-unsigned)` methods. Indexed values are `unsigned`; indexed
assignment accepts `int` or `unsigned`. Bounds and allocation failures are
checked by the runtime.

`handle` is an opaque, nullable native/runtime handle. It has no language
operators, members, or automatic ownership policy; a library binding must
define how a handle is created and released. `buffer` and `handle` values can
be stored in collections and passed through `any`. See `primary`,
`index-or-slice-suffix`, and `postfix`.

```simp
// Complete program: byte-buffer literals, access, and mutation.
// test: {"stdout": "365"}
start {
    buffer bytes = buffer[65, 66]
    bytes.append(67)
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
type descriptors, class references, lists, dicts, buffers, handles, and
`null`. Values are
stored with dynamic tags, but `any` is not a type that can be declared for an
element, variable, field, parameter, or return.
`list.length` is read-only; `append(value)` and `resize(int)` mutate it.
Type descriptors are also accepted by `append`. Although literals accept
buffers and handles, the current `append` semantic check does not accept those
two element types directly. There is no list
element deletion operation; dicts separately provide `remove(string)`.
Indexing and foreach iteration produce values inferred as `any` internally,
not declared `any` variables. A dynamic value can be type-tested, passed,
printed, or compared with `null`; extract it into a concrete local before
using arithmetic. For class members, either extract into a typed local or
use a checked cast such as `(values[0] as Foo).method()`.
The runtime checks extraction.
List slices return a copy and can include a step:
`values[start:end:step]`. Bounds are checked by the runtime.

Dict literals use `{ key: value, ... }`. Keys must be strings; values may be
supported scalar/reference/collection types, including type descriptors.
Indexed reads and foreach
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
// test: {"stdout": "42"}
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
references) and reclaims unreachable objects. A class `destroy { ... }`
definition is used as a finalizer when the collector reclaims an instance;
collection timing is not deterministic, so finalizers should not be used as a
substitute for explicit resource management. Finalizers must not let
exceptions escape.
Explicit `object.destroy()` runs the method but does not immediately reclaim
the object's storage. It marks the object destroyed before running the
destructor chain; a repeated call or later use raises a catchable runtime
exception, and GC will not run the destructor chain a second time.

Derived and base destruction is handled by the runtime destructor chain,
most-derived first and then through its bases. Unreachable objects are
finalized during garbage collection, whose timing is nondeterministic.
Resources requiring prompt release should be closed explicitly through their
library API. See `destructor-declaration` and `postfix`, and
[STDLIB.md](STDLIB.md).

```simp
// Fragment: finalizer timing is controlled by garbage collection.
class Resource {
    destroy {
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

`inline { ... }` embeds raw C in a method, constructor, destructor, or start
block. `inline (type name, ...)` captures existing locals/parameters first,
then accessible instance fields of the current receiver. There are no static
field captures, and `start` has no instance receiver. Capture types must match
exactly; inaccessible, unavailable, and ambiguous inherited fields are errors.
Use `type .field` to bypass a local that shadows a field, or `type Base.field`
and `type Base.OtherBase.field` to select an inheritance path. Qualification
uses the existing implicit-this/base access rules, not arbitrary object
expressions. The final field name becomes the C parameter name; two captures
with the same final name are rejected even if their paths differ.

Captures are mutable references to the actual storage slot, not copies.
For example, `*count += 1` in C updates the captured Simple `int`. Managed
reference slots remain traced through rooted locals or the rooted receiver;
buffers use `SimpBuffer **`, other managed references use `void **`.
Primary, secondary, repeated nonvirtual, and shared virtual bases use the same
receiver adjustment as Simple field assignments. The declare-and-capture
sugar below still declares a **local**, never a field.

```simp
// Fragment: the declared local is implicitly captured by this inline C block.
int result inline {
    *result = 42;
}
```

```simp
class Counter {
    // test: {"stdout": "101"}
    int count
    Counter() { count = 0 }
    void add(int count) {
        inline (int .count) { *count += 1; }
        // The parameter still shadows the field in ordinary unqualified access.
        print(count)
    }
}
start {
    Counter counter = Counter()
    counter.add(10)
    print(counter.count) // 1
}
```

### Application-facing inline API

Application developers do not need runtime implementation source. Prefer the
Simple standard-library class wrappers for ordinary code. Inline C automatically
receives the installed, supported opaque C facade `simp/Stdlib.h`; it exposes
the existing native implementations of the eight shipped standard packages
without inspecting private objects. See [the standard library C API](STDLIB.md#inline-c-api)
for the symbol mapping, errors, lifetime rules, and limitations. This is a real
C bridge to those functions, **not** a facility for calling arbitrary Simple
methods or wrapper constructors from C.

Generated shims also include `stdlib.h`, `stdio.h`, `string.h`, `errno.h`,
`ctype.h`, `stdint.h`, `limits.h`, and `unistd.h` once per translation unit,
before all C bodies. Their normal C APIs are available without user includes.
`unistd.h` is **POSIX-only**: the current compiler/runtime build requires a
POSIX host/target with that header, and CMake rejects its absence. If the Clang
target lacks any required header, shim compilation fails with a diagnostic;
headers are never silently omitted. The compiler uses its configured native
Clang target, not a separate inline-C cross-compilation target.

Inline C must preserve the language's value representation and GC invariants.
`malloc` memory is not managed Simple storage; do not put it into a String,
collection, buffer, or class reference slot. Raw native resources belong in
`handle` slots and require explicit cleanup. Do not retain capture addresses
or string-conversion pointers beyond the block, or leave the raw body with
`return`, `longjmp`, or another transfer that bypasses shim cleanup.
Legacy runtime declarations retained by the capture shim for compatibility
are not an application API promise; do not depend on private layouts or other
`Runtime*.h` interfaces.
