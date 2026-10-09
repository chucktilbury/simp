# Cwhip grammar

This document describes the syntax accepted by the current compiler, not the
larger language proposed in the design notes. The notation below is
ISO-style EBNF: `::=` means “is”, `|` means “or”, `{ X }` means zero or more
repetitions, `[ X ]` means optional, and quoted text is a literal token.
Uppercase names such as `IDENT` and `NEWLINE` are lexical token classes.
Comments in the productions are explanatory, not grammar terminals.

## Lexical grammar

The lexer recognizes ASCII identifiers and keywords, UTF-8 string contents,
and the tokens listed here. Keywords are case-insensitive except that the
exact spelling `String` is an identifier (the builtin class); keyword-like
spellings such as `STRG` are still the `strg` type token. Lowercase `string`
is an ordinary identifier.

```ebnf
LETTER          ::= "A" | "B" | "C" | "D" | "E" | "F" | "G" | "H" | "I"
                  | "J" | "K" | "L" | "M" | "N" | "O" | "P" | "Q" | "R"
                  | "S" | "T" | "U" | "V" | "W" | "X" | "Y" | "Z"
                  | "a" | "b" | "c" | "d" | "e" | "f" | "g" | "h" | "i"
                  | "j" | "k" | "l" | "m" | "n" | "o" | "p" | "q" | "r"
                  | "s" | "t" | "u" | "v" | "w" | "x" | "y" | "z" ;
DIGIT           ::= "0" | "1" | "2" | "3" | "4" | "5" | "6" | "7" | "8" | "9" ;
IDENT_START     ::= LETTER | "_" ;
IDENT_PART      ::= IDENT_START | DIGIT ;
IDENT           ::= IDENT_START, { IDENT_PART } ;
QUALIFIED_IDENT ::= IDENT, { ".", IDENT } ;

DECIMAL_INTEGER ::= DIGIT, { DIGIT } ;
HEX_DIGIT       ::= DIGIT | "a" | "b" | "c" | "d" | "e" | "f"
                  | "A" | "B" | "C" | "D" | "E" | "F" ;
HEX_INTEGER     ::= ("0x" | "0X"), HEX_DIGIT, { HEX_DIGIT } ;
INTEGER         ::= DECIMAL_INTEGER | HEX_INTEGER ;
UNSIGNED_INT    ::= INTEGER, ("u" | "U") ;
EXPONENT        ::= ("e" | "E"), [ "+" | "-" ], DIGIT, { DIGIT } ;
FLOAT           ::= DIGIT, { DIGIT }, ".", { DIGIT }, [ EXPONENT ]
                  | ".", DIGIT, { DIGIT }, [ EXPONENT ]
                  | DIGIT, { DIGIT }, EXPONENT ;

DOUBLE_STRING   ::= '"', { DOUBLE_CHAR | ESCAPE }, '"' ;
SINGLE_STRING   ::= "'", { SINGLE_CHAR }, "'" ;
ESCAPE          ::= "\", ("e" | "n" | "r" | "t" | "\" | '"') ;
DOUBLE_CHAR     ::= any valid UTF-8 character except '"', "\", CR, or LF ;
SINGLE_CHAR     ::= any valid UTF-8 character except "'", CR, or LF ;
STRING          ::= DOUBLE_STRING | SINGLE_STRING ;

COMMENT         ::= LINE_COMMENT | BLOCK_COMMENT ;
LINE_COMMENT    ::= ";" , { any character except LF }
                  | "#" , { any character except LF }
                  | "//", { any character except LF } ;
BLOCK_COMMENT   ::= "/*", { any character or LF except the first "*/" }, "*/" ;
HORIZONTAL_WS   ::= one or more non-LF characters for which the C++ runtime
                    classifies `isspace` as true ;
NEWLINE         ::= LF ;

PUNCTUATION     ::= "{" | "}" | "(" | ")" | "[" | "]" | "," | "." | ":" ;
OPERATOR        ::= "+=" | "-=" | "*=" | "/=" | "%="
                  | "+" | "-" | "*" | "/" | "%" | "!" | "&&" | "||"
                  | "=" | "==" | "!=" | "<" | "<=" | ">" | ">=" ;
```

The reserved words are `start`, `int`, `bool`, `float`, `unsigned`, `strg`, `callback`,
`list`, `dict`, `buffer`, `handle`, `any`, `type`, `class`,
`namespace`, `include`, `inline`, `import`, `as`, `public`, `protected`,
`private`, `virtual`, `super`, `null`, `true`, `false`, `return`, `void`,
`if`, `else`, `while`, `do`, `for`, `in`, `is`, `break`, `continue`, `and`,
`or`, `not`, `print`, `raise`, `try`, `except`, `finally`, and `from`.

Unsuffixed decimal and hexadecimal integer literals have type `int`; a trailing
`u` or `U` gives either spelling type `unsigned`. Hexadecimal digits are
case-insensitive. Signs are unary operators: `-0x8000000000000000` is the
signed minimum, while the corresponding positive literal is outside the
signed range. Literal magnitudes must fit the selected signed or unsigned
64-bit range.

Lexical details that affect parsing:

- A leading `+` or `-` is an operator, not part of a number. Unsuffixed
  decimal and hexadecimal integer values are signed 64-bit; `u`/`U` marks an
  unsigned 64-bit literal. Values outside the selected type's range are
  rejected. Floats accept leading-dot (`.5`),
  trailing-dot (`5.`), and exponent forms (`1e3`, `1.0E-3`); an exponent must
  contain digits.
- Only double-quoted strings interpret escapes: `\e` produces ESC (`0x1b`),
  and `\n`, `\r`, `\t`, `\\`, and `\"` are also accepted. Single-quoted
  strings preserve backslashes literally. Both quote styles are one physical
  line and must decode to valid
  UTF-8. Neither quote style interpolates automatically; literal templates
  are interpreted only by `format` or multi-argument `print`.
- `;`, `#`, and `//` begin line comments; a semicolon is **not** a statement
  separator. Block comments are non-nesting. Newline characters inside a
  block comment still delimit statements outside parentheses and brackets.
- LF produces a `NEWLINE` token except while inside parentheses or square
  brackets. Newlines inside braces are not suppressed. Other whitespace is
  discarded. There is no indentation-based syntax.
- `&` and `|` are legal only as `&&` and `||`. The scalar compound-assignment
  operators are `+=`, `-=`, `*=`, `/=`, and `%=`. There are no increment,
  bitwise, or conditional (`?:`) operators.
- After `inline` and an optional capture list, the lexer consumes the next
  balanced-brace block as one opaque `INLINE_BODY` token. Braces inside C
  strings and C comments do not change its brace depth.
- `INLINE_BODY` is a lexer-only token: its contents are C source rather than
  Simple tokens, so it has no ordinary Simple lexical production.

## Syntactic grammar

`include` is handled by the source loader before the parser runs; it is shown
below as source-level syntax, not as a declaration accepted by
`Parser::parseProgram`. `EOF` is the lexer end token. `terminator` reflects
the parser's newline rule, including its allowance for a closing brace or
end-of-file in place of a final newline.

### Compilation units and declarations

```ebnf
program             ::= { NEWLINE }, { top-level-item, { NEWLINE } },
                        start-block, { NEWLINE }, EOF ;
module              ::= { NEWLINE }, { module-item, { NEWLINE } }, EOF ;

top-level-item      ::= import-declaration
                      | class-declaration
                      | namespace-declaration
                      | out-of-line-definition ;
module-item         ::= import-declaration
                      | class-declaration
                      | namespace-declaration
                      | out-of-line-definition ;

start-block         ::= "start", { NEWLINE }, block ;
import-declaration  ::= "import", IDENT, [ "as", IDENT ], terminator ;
namespace-declaration
                    ::= "namespace", IDENT, { NEWLINE }, "{",
                        { NEWLINE },
                        { namespace-item, { NEWLINE } },
                        "}" ;
namespace-item      ::= namespace-declaration
                      | class-declaration
                      | out-of-line-definition ;

class-declaration   ::= "class", IDENT, [ ":", base-specifier,
                        { ",", base-specifier } ], "{",
                        { NEWLINE }, { class-member, { NEWLINE } }, "}" ;
base-specifier      ::= { base-modifier }, QUALIFIED_IDENT ;
base-modifier       ::= "public" | "protected" | "private" | "virtual" ;

class-member        ::= access-section
                      | field-declaration
                      | anonymous-enum-declaration
                      | constructor-declaration
                      | destructor-declaration
                      | method-declaration ;
access-section      ::= ("public" | "protected" | "private"), ":", { NEWLINE } ;
field-declaration   ::= type, IDENT, terminator ;
anonymous-enum-declaration
                    ::= "enum", "{", { NEWLINE },
                        [ enum-member,
                          { (",", { NEWLINE } | NEWLINE), enum-member },
                          [ "," ], { NEWLINE } ],
                        "}" ;
enum-member         ::= IDENT, [ "=", expression ] ;
constructor-declaration
                    ::= IDENT, parameter-list, method-tail ;
destructor-declaration
                    ::= "destroy", block ;
method-declaration  ::= type, IDENT, parameter-list, method-tail ;
method-tail         ::= block | terminator ;

out-of-line-definition
                    ::= return-type, QUALIFIED_IDENT, ".", IDENT,
                        parameter-list,
                        ( "from", STRING, terminator | block ) ;
return-type         ::= type | "void" ;
parameter-list      ::= "(", [ parameter, { ",", parameter } ], ")" ;
parameter           ::= type, IDENT ;
type                ::= primitive-type | QUALIFIED_IDENT | callback-type ;
callback-type       ::= "callback", "<", return-type, "(",
                        [ type, { ",", type } ], ")", ">" ;
primitive-type      ::= "int" | "bool" | "float" | "unsigned" | "strg"
                      | "list" | "dict" | "buffer"
                      | "handle" | "any" | "type" ;

include-directive   ::= "include", DOUBLE_STRING, terminator ;
```

`destroy` is an in-class declaration form, not a general method signature: it
must be followed by a block and has no return type or parameter list.
Destructors cannot be declared without a body or defined out of line.
Callback signatures have unnamed parameter types. A member expression without
invocation parentheses captures an instance method when it resolves to a method
rather than a field; the expected callback type resolves overloads. A call's
callee may also be a typed callback value. See [CALLBACKS.md](CALLBACKS.md) for
the semantic restrictions and native ABI.
`enum` is contextual to the class-member production above; it does not
introduce a named enum type or a top-level declaration.

The source loader accepts `include-directive` only at brace depth zero and
before the root `start`; it searches the including file's directory before
configured include paths, expands each canonical file once, and rejects
`start` inside included text. Imports are parsed but resolved through the
module/package registry during compilation. Without an alias the binding uses
the manifest's declared export name. An explicit import alias denotes the
exported namespace itself; it does not add a package-name prefix to exported
class names (for example, `import time as T` exposes `T.Clock`). Imported
module files use `module`, which does not permit a `start` block.

### Statements

```ebnf
block               ::= "{", { NEWLINE }, { statement, { NEWLINE } }, "}" ;
statement           ::= declaration
                      | assignment
                      | method-call-statement
                      | print-statement
                      | if-statement
                      | while-statement
                      | do-while-statement
                      | foreach-statement
                      | loop-control
                      | return-statement
                      | try-statement
                      | raise-statement
                      | super-initializer
                      | inline-c-statement
                      | block ;

declaration         ::= type, IDENT,
                        [ "=", expression
                        | direct-constructor-arguments ],
                        terminator
                      | type, IDENT, inline-c-statement ;
direct-constructor-arguments
                    ::= "(", [ expression, { ",", expression } ], ")" ;
assignment          ::= expression, assignment-operator, expression, terminator ;
assignment-operator ::= "=" | "+=" | "-=" | "*=" | "/=" | "%=" ;
method-call-statement
                    ::= expression, terminator ;
print-statement     ::= "print", "(", [ expression
                         | STRING, ",", format-argument-list ], ")", terminator ;
if-statement        ::= "if", "(", expression, ")", { NEWLINE }, block,
                        [ { NEWLINE }, "else", { NEWLINE }, block ] ;
while-statement     ::= "while", "(", expression, ")", { NEWLINE }, block ;
do-while-statement  ::= "do", { NEWLINE }, block, { NEWLINE },
                        "while", "(", expression, ")" ;
foreach-statement   ::= "for", "(", IDENT, [ ",", IDENT ], "in",
                        expression, ")", { NEWLINE }, block ;
loop-control        ::= ("break" | "continue"), terminator ;
return-statement    ::= "return", [ expression | "(", ")" ], terminator ;
raise-statement     ::= "raise", "(", [ expression ], ")", terminator ;
try-statement       ::= "try", { NEWLINE }, block, { NEWLINE },
                        ( except-clause, { NEWLINE },
                          { except-clause, { NEWLINE } },
                          [ "finally", { NEWLINE }, block, { NEWLINE } ]
                        | "finally", { NEWLINE }, block, { NEWLINE } ) ;
except-clause       ::= "except", "(", [ QUALIFIED_IDENT ], ")",
                        [ "as", IDENT ], { NEWLINE }, block ;
super-initializer   ::= ( "super", [ "virtual" ] | "virtual", "super" ), IDENT,
                        "(", [ expression, { ",", expression } ], ")",
                        terminator ;
inline-c-statement  ::= "inline", [ capture-list ], INLINE_BODY, terminator ;
capture-list        ::= "(", [ capture, { ",", capture } ], ")" ;
capture             ::= capture-type, [ "." ], IDENT, { ".", IDENT } ;
capture-type        ::= type | "void" ;
terminator          ::= NEWLINE, { NEWLINE } | ε ;
```

`terminator ::= ε` is valid only immediately before `}` or `EOF`; it does not
allow adjacent simple statements on the same line. The declaration form that
places `inline` after a type and identifier declares that local and adds it
as an implicit capture. An initializer on that form is rejected.
An unqualified capture resolves a local/parameter before an instance field.
Leading `.` selects the implicit receiver explicitly; other dotted capture
paths must resolve through that receiver's base classes. Only the final
identifier names the C parameter; final names must be unique within a list.
Field captures obey normal access, ambiguity, and exact-type checks and
require an instance context.
`direct-constructor-arguments` is accepted only when `type` is a class name
(a `QUALIFIED_IDENT`), not a built-in type such as `int`, `strg`, or `buffer`.
It constructs the declared class directly; it does not declare a function.
Class fields still have no initializer syntax.

### Expressions

The first table gives the exact recursive-descent levels from lowest to
highest binding strength. The loop-based levels associate left-to-right;
unary operators associate right-to-left.

| Precedence | Operators / form | Associativity |
|---:|---|---|
| 1 | `or`, `\|\|` | left |
| 2 | `and`, `&&` | left |
| 3 | `not`, `!` | right (prefix) |
| 4 | `==`, `!=` | left |
| 5 | `is`, `<`, `<=`, `>`, `>=` | left |
| 6 | `+`, `-` | left |
| 7 | `*`, `/`, `%` | left |
| 8 | unary `+`, unary `-` | right (prefix) |
| 9 | member access, calls, indexing, slicing, `as` checked extraction | repeated left-to-right |

`is` consumes a type name rather than a right-hand expression. Equality
operators bind less tightly than `is` and relational operators because that
is how `parseComparison` and `parseRelational` are layered. Assignment is a
statement, not an expression.

```ebnf
expression          ::= or-expression ;
or-expression       ::= and-expression, { ("or" | "||"), and-expression } ;
and-expression      ::= not-expression, { ("and" | "&&"), not-expression } ;
not-expression      ::= ("not" | "!"), not-expression | comparison ;
comparison          ::= relational, { ("==" | "!="), relational } ;
relational          ::= addition,
                        { ("is", type-test-name | "<", addition
                          | "<=", addition | ">", addition | ">=", addition) } ;
type-test-name      ::= QUALIFIED_IDENT | primitive-type | "void" ;
addition            ::= multiplication, { ("+" | "-"), multiplication } ;
multiplication      ::= unary, { ("*" | "/" | "%"), unary } ;
unary               ::= ("+" | "-"), unary | postfix ;

postfix             ::= primary, { member-suffix | index-or-slice-suffix
                                  | call-suffix | object-cast-suffix } ;
object-cast-suffix  ::= "as", cast-target ;
cast-target         ::= QUALIFIED_IDENT | primitive-type | "void" | "null" ;
member-suffix       ::= ".", IDENT ;
index-or-slice-suffix
                    ::= "[", expression, [ ":", [ expression ],
                        [ ":", [ expression ] ] ], "]"
                      | "[", ":", [ expression ], [ ":", [ expression ] ], "]" ;
call-suffix         ::= "(", [ argument-list ], ")" ;
format-expression   ::= "format", "(", STRING, [ ",", format-argument-list ], ")" ;
format-argument-list
                    ::= argument-list
                      | named-format-argument,
                         { ",", named-format-argument } ;
named-format-argument
                    ::= IDENT, "=", expression ;
argument-list       ::= expression, { ",", expression } ;

primary             ::= INTEGER | UNSIGNED_INT | FLOAT | STRING
                      | format-expression
                      | "true" | "false" | "null" | IDENT
                      | "type", "(", expression, ")"
                      | "bool", "(", expression, ")"
                      | "int", "(", expression, ")"
                      | "unsigned", "(", expression, ")"
                      | "float", "(", expression, ")"
                      | "buffer", "(", [ expression, { ",", expression } ], ")"
                      | buffer-literal
                      | "(", expression, ")"
                      | list-literal | dict-literal | type-value ;
buffer-literal      ::= "buffer", "[", [ argument-list ], "]" ;
list-literal        ::= "[", [ argument-list ], "]" ;
dict-literal        ::= "{", [ dict-entry, { ",", dict-entry } ], "}" ;
dict-entry          ::= expression, ":", expression ;
type-value          ::= "strg" | "list" | "dict"
                      | "handle" | "any"
                      | "bool" | "int" | "unsigned" | "float"  (* only when not followed by "(" *)
                      | "buffer"                      (* only when not followed by "(" or "[" *) ;
```

The `postfix` shorthand accepts a call suffix after any value expression.
An identifier call is initially parsed as construction and resolves instead
to invocation when it names a callback variable or field. A member call
resolves to a method call or callback-field invocation. A returned callback
can be invoked directly, as in `object.capture()(argument)`; attempting to
call any non-callable value is diagnosed.
An `as` suffix accepts a supported non-void target and a value with a valid
checked-extraction source type. It binds at postfix precedence and can be
repeated.
Parenthesize the cast before member access, as in
`(items[index] as model.Foo).method()`, because dotted names after `as` are
parsed as qualified class names. Scalar conversion retains `int(value)` and
the other scalar type-name call forms; `Foo(value)` constructs a class unless
`Foo` resolves to a callback value.
Name resolution may recognize a dotted qualified class construction. A
member expression such as `Base.field` or `Left.Root.method(args)` inside a
class method, constructor, or destructor may instead receive an implicit
current-object receiver when its initial class name denotes a direct base.
The remaining qualifiers must follow successive direct-base edges, as with
explicit `object.Left.Root.field` access. This is semantic resolution of the
existing postfix grammar, not static member syntax or a new parser production.
Anonymous enum constants also support `Class.NAME` and `object.NAME`;
class-qualified constant reads are not static fields or static method calls.
Local/parameter and field receiver names take precedence over implicit base
qualification; namespace/import class lookup otherwise follows ordinary
lexical rules. Type-valued name resolution is unchanged. A
`method-call-statement` must resolve to a method- or callback-call AST node; a bare
constructor call is not accepted as a statement. Identifier- and
parenthesis-started expressions are routed through statement parsing;
parenthesized receivers such as `(items[0] as Foo).method()` are valid
standalone calls, including calls returning `void`. Parenthesizing a noncall
does not make it a valid expression statement. Declaration and assignment
recognition still applies before the method-call-only check.
Member, index, and callable invocation suffixes may follow a call.
The `format` expression requires a literal template,
returns `strg`, and accepts either
positional expressions or named `IDENT=expression` arguments, never both.
Positional `{}` placeholders match positional arguments in order. Named
`{IDENT}` placeholders match named arguments by case-sensitive name; every
distinct placeholder name must have exactly one argument, and every argument
must be used. A named placeholder may appear more than once and reuses its
argument value. `{{` and `}}` produce literal braces; unmatched braces and
malformed placeholder names are compile-time errors. Fields may include
`:` followed by `[alignment][width][type]`, where alignment is `<`, `>`, or `^`,
width is decimal digits (maximum 1000000), and type is `d`, `x`, `X`, or `c`.
A leading-zero width requests integral zero padding; explicit alignment and
`c` cannot combine with it. See the language reference for type, ASCII,
padding, and exception rules. Validation happens only in `format` or
multi-argument `print`. String-literal calls are rejected.
Indexing and slicing bind as postfix forms. `type` always
starts `type(expression)`; it is not a standalone type-value expression.

## Parser coverage map

Each parser routine has a corresponding production or grammar note above:

| Parser routine | Production(s) / role |
|---|---|
| `parseProgram`, `parseModule` | `program`, `module`; exactly one `start` only in a program |
| `parseImport`, `parseQualifiedIdentifier`, `parseNamespace` | `import-declaration`, `QUALIFIED_IDENT`, `namespace-declaration` |
| `parseBlock`, `parseStatement` | `block`, `statement` |
| `parseInlineC`, `parseDeclaration`, `parseExpressionStatement` | `inline-c-statement`, `declaration`, `assignment`, `method-call-statement` |
| `parseReturn`, `parseSuperConstructorCall`, `parsePrint` | `return-statement`, `super-initializer`, `print-statement` |
| `parseIf`, `parseWhile`, `parseDoWhile`, `parseForEach`, `parseLoopControl` | matching statement productions |
| `parseRaise`, `parseTry` | `raise-statement`, `try-statement` |
| `parseExpression`, `parseOr`, `parseAnd`, `parseNot`, `parseComparison`, `parseRelational`, `parseAddition`, `parseMultiplication`, `parseUnary` | matching expression-precedence productions |
| `parseTypeTestName`, `parsePrimary`, `parseTypeName`, `parsePostfix` | `type-test-name`, `primary`, type-value primary, and postfix productions |
| `startsOutOfLineDefinition` | top-level lookahead for `out-of-line-definition`; not a separate syntax form |
| `parseClass`, `parseEnumMembers`, `parseOutOfLineMethodDefinition`, `parseType`, `parseParameters`, `parseMethod` | class/member/declaration productions, including `anonymous-enum-declaration` |
| `Parser` constructor, `current`, `previous`, `check`, `match`, `error` | token-stream setup/access, predicates, cursor movement, and diagnostics; these do not add productions |
| `skipNewlines`, `consumeStatementTerminator`, `consume`, `parseFormatArguments`, `validateFormatString`, `trace` | lexical/newline policy, `terminator`, positional/named format arguments and validation, and tracing |

The map covers syntax-producing methods in `Parser.cpp` and `ParserClass.cpp`
and identifies parser infrastructure and validation helpers. The
`tests/functional/positive/*.simp` fixtures
exercise scalar and float literal variants, declarations and assignments,
calls and overloads, casts and type tests, lists/dicts/buffers/handles,
indexing/slicing/iteration, conditionals and loops, classes and inheritance
(including anonymous enums, virtual and secondary bases), exceptions,
namespaces, imports, textual inclusion, native bindings, inline C, strings,
and thread bindings.
`include-directive` case is the only construct expanded before parsing.

## Notes and known irregularities

- Keywords are case-folded, but ordinary identifiers are case-sensitive.
  `String` is an identifier naming the builtin class, `strg` is the primitive
  string type keyword, and lowercase `string` is an ordinary identifier.
- `array` and `map` are ordinary identifiers, not collection type aliases.
- Newline suppression tracks both `()` and `[]` with one nesting counter. A
  mismatched closer can therefore affect whether later newlines are emitted;
  syntax diagnostics, not the lexer, report unmatched delimiters.
- Base modifiers may appear in any order and repeated access modifiers use
  the last one; repeating `virtual` is a parse error. Class member access
  begins as public and changes only at an access section.
- `else if` has no dedicated form; `else` must be followed by a block.
  `do { ... } while (...)` has no required trailing terminator.
- Newlines in `()` and `[]` are suppressed, but newlines inside `{}` are not.
  In particular, braces used for dict literals do not provide multiline
  continuation.
- `return ()` is accepted as a void return, alongside bare `return`.
  Parenthesized non-empty returns are ordinary expressions.
- `format` is recognized as an intrinsic in call position. `print` formats
  only when its literal first argument has additional arguments; otherwise
  a directly printed literal, including braces, is unchanged.
- A base initializer is spelled `super Base(args)`. A virtual-base initializer
  accepts either `super virtual Base(args)` or `virtual super Base(args)`;
  dotted forms are not part of the grammar.
  A constructor's leading initializer sequence may be enclosed in one ordinary
  `try-statement`. Its body must contain only ordered `super-initializer`
  statements, with no branches, loops, nested blocks, or ordinary statements.
  Required direct initializers must all be inside that sequence. Handlers must
  raise/rethrow on every path and contain no return; optional `finally` follows
  ordinary cleanup rules. No initializer is allowed in a handler or `finally`,
  after this leading try, or in a mixed direct/protected sequence. These are
  semantic constraints, not new parser productions.
- The parser allows some forms that later fail semantic analysis: for example
  `return` in `start` or within `finally` (including nested blocks),
  `break` outside a loop, invalid lvalues, inaccessible
  members, unknown types, bad argument/return conversions, an invalid
  `super` initializer, or a `raise` expression outside the exception class
  hierarchy. Constructor declarations must use their enclosing class name and omit a
  return type; a class-body `method-declaration` whose `IDENT` is the
  enclosing class name (for example `void Name()`) is a parser error;
  destructor parameters are rejected; native-bound constructors are rejected;
  and constructor/destructor placement and leading base-initializer rules are
  semantic checks.
- `void` is allowed by the type parser only where requested for a method
  return or inline-C capture syntax; fields, parameters, and locals do not
  have a meaningful `void` type. (Inline-C capture validation rejects it.)
- Direct identifier calls are constructed as constructor calls by the parser;
  dotted class names can be reclassified after semantic name resolution.
  There are no free-function declarations or arbitrary first-class callable
  expressions in the accepted subset.
- The grammar allows list literals to contain buffers and handles, while the
  semantic check for `list.append(...)` accepts a narrower list of element
  types. The syntax is shared, but those two construction routes are not
  type-equivalent in every case.
