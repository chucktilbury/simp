# Simple grammar

This document describes the syntax accepted by the current compiler, not the
larger language proposed in the design notes. The notation below is
ISO-style EBNF: `::=` means “is”, `|` means “or”, `{ X }` means zero or more
repetitions, `[ X ]` means optional, and quoted text is a literal token.
Uppercase names such as `IDENT` and `NEWLINE` are lexical token classes.
Comments in the productions are explanatory, not grammar terminals.

## Lexical grammar

The lexer recognizes ASCII identifiers and keywords, UTF-8 string contents,
and the tokens listed here. Keywords are case-insensitive except that the
exact spelling `String` is an identifier (the prelude class); keyword-like
spellings such as `STRING` are still the lowercase `string` type token.

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

INTEGER         ::= DIGIT, { DIGIT } ;
UNSIGNED_INT    ::= INTEGER, ("u" | "U") ;
EXPONENT        ::= ("e" | "E"), [ "+" | "-" ], DIGIT, { DIGIT } ;
FLOAT           ::= DIGIT, { DIGIT }, ".", { DIGIT }, [ EXPONENT ]
                  | ".", DIGIT, { DIGIT }, [ EXPONENT ]
                  | DIGIT, { DIGIT }, EXPONENT ;

DOUBLE_STRING   ::= '"', { DOUBLE_CHAR | ESCAPE }, '"' ;
SINGLE_STRING   ::= "'", { SINGLE_CHAR }, "'" ;
ESCAPE          ::= "\", ("n" | "r" | "t" | "\" | '"') ;
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
OPERATOR        ::= "+" | "-" | "*" | "/" | "%" | "!" | "&&" | "||"
                  | "=" | "==" | "!=" | "<" | "<=" | ">" | ">=" ;
```

The reserved words are `start`, `int`, `bool`, `float`, `unsigned`, `string`,
`array`/`list`, `map`/`dict`, `buffer`, `handle`, `any`, `type`, `class`,
`namespace`, `include`, `inline`, `import`, `as`, `public`, `protected`,
`private`, `virtual`, `super`, `null`, `true`, `false`, `return`, `void`,
`if`, `else`, `while`, `do`, `for`, `in`, `is`, `break`, `continue`, `and`,
`or`, `not`, `print`, `raise`, `try`, `except`, `finally`, and `from`.

Lexical details that affect parsing:

- A leading `+` or `-` is an operator, not part of a number. Decimal integers
  have no radix prefix, separators, or suffix other than `u`/`U` for unsigned.
  Floats accept leading-dot (`.5`), trailing-dot (`5.`), and exponent forms
  (`1e3`, `1.0E-3`); an exponent must contain digits.
- Only double-quoted strings interpret escapes, and only `\n`, `\r`, `\t`,
  `\\`, and `\"` are accepted. Single-quoted strings preserve backslashes
  literally. Both quote styles are one physical line and must decode to valid
  UTF-8. A double-quoted string is marked as a possible format string; it is
  not interpolated unless it is immediately followed by a parenthesized
  argument list (see `format-suffix`).
- `;`, `#`, and `//` begin line comments; a semicolon is **not** a statement
  separator. Block comments are non-nesting. Newline characters inside a
  block comment still delimit statements outside parentheses and brackets.
- LF produces a `NEWLINE` token except while inside parentheses or square
  brackets. Newlines inside braces are not suppressed. Other whitespace is
  discarded. There is no indentation-based syntax.
- `&` and `|` are legal only as `&&` and `||`. There are no increment,
  compound-assignment, bitwise, or conditional (`?:`) operators.
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
import-declaration  ::= "import", IDENT, "as", IDENT, terminator ;
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
                      | constructor-declaration
                      | destructor-declaration
                      | method-declaration ;
access-section      ::= ("public" | "protected" | "private"), ":", { NEWLINE } ;
field-declaration   ::= type, IDENT, terminator ;
constructor-declaration
                    ::= IDENT, parameter-list, method-tail ;
destructor-declaration
                    ::= [ "void" ], "destroy", parameter-list, method-tail ;
method-declaration  ::= type, IDENT, parameter-list, method-tail ;
method-tail         ::= block | terminator ;

out-of-line-definition
                    ::= return-type, QUALIFIED_IDENT, ".", IDENT,
                        parameter-list,
                        ( "from", STRING, terminator | block ) ;
return-type         ::= type | "void" ;
parameter-list      ::= "(", [ parameter, { ",", parameter } ], ")" ;
parameter           ::= type, IDENT ;
type                ::= primitive-type | QUALIFIED_IDENT ;
primitive-type      ::= "int" | "bool" | "float" | "unsigned" | "string"
                      | "array" | "list" | "map" | "dict" | "buffer"
                      | "handle" | "any" | "type" ;

include-directive   ::= "include", DOUBLE_STRING, terminator ;
```

The source loader accepts `include-directive` only at brace depth zero and
before the root `start`; it searches the including file's directory before
configured include paths, expands each canonical file once, and rejects
`start` inside included text. Imports are parsed but resolved through the
module/package registry during compilation. The import alias denotes the
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

declaration         ::= type, IDENT, [ "=", expression ], terminator
                      | type, IDENT, inline-c-statement ;
assignment          ::= expression, "=", expression, terminator ;
method-call-statement
                    ::= expression, terminator ;
print-statement     ::= "print", "(", [ expression ], ")", terminator ;
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
super-initializer   ::= "super", ".", [ "virtual" ], IDENT,
                        "(", [ expression, { ",", expression } ], ")",
                        terminator ;
inline-c-statement  ::= "inline", [ capture-list ], INLINE_BODY, terminator ;
capture-list        ::= "(", [ capture, { ",", capture } ], ")" ;
capture             ::= capture-type, IDENT ;
capture-type        ::= type | "void" ;
terminator          ::= NEWLINE, { NEWLINE } | ε ;
```

`terminator ::= ε` is valid only immediately before `}` or `EOF`; it does not
allow adjacent simple statements on the same line. The declaration form that
places `inline` after a type and identifier declares that local and adds it
as an implicit capture. An initializer on that form is rejected.

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
| 9 | member access, calls, indexing and slicing | repeated left-to-right |

`is` consumes a type name rather than a right-hand expression. Equality
operators bind more tightly than `is` and relational operators because that
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
                                  | call-suffix | format-suffix } ;
member-suffix       ::= ".", IDENT ;
index-or-slice-suffix
                    ::= "[", expression, [ ":", [ expression ],
                        [ ":", [ expression ] ] ], "]"
                      | "[", ":", [ expression ], [ ":", [ expression ] ], "]" ;
call-suffix         ::= "(", [ argument-list ], ")" ;
format-suffix       ::= "(", [ argument-list ], ")" ;
argument-list       ::= expression, { ",", expression } ;

primary             ::= INTEGER | UNSIGNED_INT | FLOAT | STRING
                      | "true" | "false" | "null" | IDENT
                      | "type", "(", expression, ")"
                      | "int", "(", expression, ")"
                      | "unsigned", "(", expression, ")"
                      | "float", "(", expression, ")"
                      | "buffer", "(", [ expression, { ",", expression } ], ")"
                      | "(", expression, ")"
                      | array-literal | map-literal | type-value ;
array-literal       ::= "[", [ argument-list ], "]" ;
map-literal         ::= "{", [ map-entry, { ",", map-entry } ], "}" ;
map-entry           ::= expression, ":", expression ;
type-value          ::= "bool" | "string" | "array" | "list" | "map" | "dict"
                      | "handle" | "any"
                      | "int" | "unsigned" | "float"  (* only when not followed by "(" *)
                      | "buffer"                      (* only when not followed by "(" *) ;
```

The `postfix` shorthand has these implementation constraints: a call suffix
is accepted only after an identifier or member expression; an identifier
call is initially a constructor call, while a member call is a method call.
Name resolution may recognize a dotted qualified class construction. A
`method-call-statement` must parse to a method-call AST node; a bare
constructor call is not accepted as a statement. Calls cannot be chained
directly after a call or constructor-call node, although member and index
suffixes can follow. A format suffix uses the same parentheses as a call, but
is accepted only on a double-quoted string literal. Its literal must contain
exactly one `{}` for each argument and no other braces; this validation runs
only when the literal is called. A single-quoted string is never a
format-call target. Indexing and slicing bind as postfix forms. `type` always
starts `type(expression)`; it is not a standalone type-value expression.

## Parser coverage map

Each parser routine has a corresponding production or grammar note above:

| Parser routine | Production(s) / role |
|---|---|
| `parseProgram`, `parseModule` | `program`, `module`; exactly one `start` only in a program |
| `parseImport`, `parseQualifiedIdentifier`, `parseNamespace` | `import-declaration`, `QUALIFIED_IDENT`, `namespace-declaration` |
| `parseBlock`, `parseStatement` | `block`, `statement` |
| `parseInlineC`, `parseDeclaration`, `parseIdentifierStatement` | `inline-c-statement`, `declaration`, `assignment`, `method-call-statement` |
| `parseReturn`, `parseSuperConstructorCall`, `parsePrint` | `return-statement`, `super-initializer`, `print-statement` |
| `parseIf`, `parseWhile`, `parseDoWhile`, `parseForEach`, `parseLoopControl` | matching statement productions |
| `parseRaise`, `parseTry` | `raise-statement`, `try-statement` |
| `parseExpression`, `parseOr`, `parseAnd`, `parseNot`, `parseComparison`, `parseRelational`, `parseAddition`, `parseMultiplication`, `parseUnary` | matching expression-precedence productions |
| `parseTypeTestName`, `parsePrimary`, `parseTypeName`, `parsePostfix` | `type-test-name`, `primary`, type-value primary, and postfix productions |
| `startsOutOfLineDefinition` | top-level lookahead for `out-of-line-definition`; not a separate syntax form |
| `parseClass`, `parseOutOfLineMethodDefinition`, `parseType`, `parseParameters`, `parseMethod` | class/member/declaration productions |
| `Parser` constructor, `current`, `previous`, `check`, `match`, `error` | token-stream setup/access, predicates, cursor movement, and diagnostics; these do not add productions |
| `skipNewlines`, `consumeStatementTerminator`, `consume`, `validateFormatString`, `trace` | lexical/newline policy, `terminator`, format restrictions, and tracing; helpers do not add productions |

All 50 member functions in `Parser.cpp` and `ParserClass.cpp` are accounted
for above: syntax-producing methods map to productions and the remaining
methods are identified as parser infrastructure or validation helpers. The
95 `tests/functional/positive/*.simp` fixtures were checked against this map:
all their parsed forms are represented by productions above. The fixtures
exercise scalar and float literal variants, declarations and assignments,
calls and overloads, casts and type tests, arrays/maps/buffers/handles,
indexing/slicing/iteration, conditionals and loops, classes and inheritance
(including virtual and secondary bases), exceptions, namespaces, imports,
textual inclusion, native bindings, inline C, strings, and thread bindings.
No positive-fixture syntax or parser routine was left unmapped. The
`include-directive` case is the only construct expanded before parsing.

## Notes and known irregularities

- Keywords are case-folded, but ordinary identifiers are case-sensitive;
  `String` is specially reserved as an identifier while the lowercase
  `string` spelling is a type keyword. This makes casing significant in an
  unusual way.
- `list` and `array` share a token, as do `dict` and `map`; their distinction
  is lost at parse time.
- Newline suppression tracks both `()` and `[]` with one nesting counter. A
  mismatched closer can therefore affect whether later newlines are emitted;
  syntax diagnostics, not the lexer, report unmatched delimiters.
- Base modifiers may appear in any order and repeated access modifiers use
  the last one; repeating `virtual` is a parse error. Class member access
  begins as public and changes only at an access section.
- `else if` has no dedicated form; `else` must be followed by a block.
  `do { ... } while (...)` has no required trailing terminator.
- Newlines in `()` and `[]` are suppressed, but newlines inside `{}` are not.
  In particular, braces used for map literals do not provide multiline
  continuation.
- `return ()` is accepted as a void return, alongside bare `return`.
  Parenthesized non-empty returns are ordinary expressions.
- `print` has an extra parser check: a directly printed double-quoted literal
  containing `{` or `}` must be used as a format call. The same literal can
  otherwise be stored or used as a normal string.
- A virtual-base initializer is spelled `super.virtual Base(args)` (the
  parser expects the `virtual` keyword after the first dot and does not
  consume another dot before the base name).
- The parser allows some forms that later fail semantic analysis: for example
  `return` in `start`, `break` outside a loop, invalid lvalues, inaccessible
  members, unknown types, bad argument/return conversions, an invalid
  `super` initializer, or a `raise` expression outside the exception class
  hierarchy. Constructor declarations must use their enclosing class name;
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
- The grammar allows array literals to contain buffers and handles, while the
  semantic check for `array.append(...)` accepts a narrower list of element
  types. The syntax is shared, but those two construction routes are not
  type-equivalent in every case.
