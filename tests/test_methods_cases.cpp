#include "test_cases.hpp"

#include "simp/Diagnostic.hpp"
#include "simp/Lexer.hpp"
#include "simp/Parser.hpp"
#include "simp/Token.hpp"

#include <algorithm>
#include <sstream>
#include <string>
#include <vector>

namespace {
using namespace simp_test;

const TestGroupRegistration registration{3, {
        {"implicit base qualifiers use the current instance", [] {
             expectValid(
                 "class A { int x\n protected:\n int p\n public:\n"
                 "int f(int n) { return n } }\n"
                 "class B { int x\n int f(int n) { return n + 1 } }\n"
                 "class C: A, private B {\n"
                 " C() { super A()\n super B()\n A.x = 1\n B.x = 2\n A.x += 3\n"
                 "print(A.x)\n print(B.f(A.p)) }\n"
                 " destroy { print(B.x) }\n"
                 "}\nstart { C c = C()\n print(c.A.f(1)) }");
         }},
        {"implicit qualifiers preserve member and path accessibility", [] {
             expectDiagnostic(
                 "class A { private:\n int x }\n"
                 "class C: A { void f() { print(A.x) } }\nstart {}",
                 "field 'x' is not accessible");
             expectDiagnostic(
                 "class A { private:\n int x }\n"
                 "class C: A { void f() { A.x += 1 } }\nstart {}",
                 "field 'x' is not accessible");
             expectDiagnostic(
                 "class A { private:\n int x }\n"
                 "class C: A { void f() { A.x = 1 } }\nstart {}",
                 "field 'x' is not accessible");
             expectDiagnostic(
                 "class A { private:\n void f() {} }\n"
                 "class C: A { void g() { A.f() } }\nstart {}",
                 "method 'f' is not accessible");
             expectDiagnostic(
                 "class A { protected:\n int x }\n"
                 "class C: A {}\nstart { C c = C()\n print(c.A.x) }",
                 "field 'x' is not accessible");
             expectDiagnostic(
                 "class A { protected:\n void f() {} }\n"
                 "class C: A {}\nstart { C c = C()\n c.A.f() }",
                 "method 'f' is not accessible");
             expectDiagnostic(
                 "class A { int x }\nclass C: private A {}\n"
                 "start { C c = C()\n print(c.A.x) }",
                 "field 'x' is not accessible through this inheritance path");
             expectDiagnostic(
                 "class A { void f() {} }\nclass C: private A {}\n"
                 "start { C c = C()\n c.A.f() }",
                 "method 'f' is not accessible through this inheritance path");
             expectDiagnostic(
                 "class A { int x }\nclass B: private A {}\n"
                 "class C: B { void f() { print(B.A.x) } }\nstart {}",
                 "field 'x' is not accessible through this inheritance path");
             expectDiagnostic(
                 "class A { void f() {} }\nclass B: private A {}\n"
                 "class C: B { void g() { B.A.f() } }\nstart {}",
                 "method 'f' is not accessible through this inheritance path");
         }},
        {"implicit qualifiers do not hide ambiguity or create static access", [] {
             expectDiagnostic(
                 "class A { int x }\nclass B { int x }\n"
                 "class C: A, B { void f() { print(x) } }\nstart {}",
                 "ambiguous inherited field 'x'");
             expectDiagnostic(
                 "class A { void f() {} }\nclass B { void f() {} }\n"
                 "class C: A, B { void g() { f() } }\nstart {}",
                 "ambiguous inherited method 'f'");
             expectDiagnostic(
                 "class A { int x }\nstart { print(A.x) }",
                 "undefined variable 'A'");
             expectDiagnostic(
                 "class A { void f() {} }\nstart { A.f() }",
                 "unknown qualified class 'A.f'");
             expectDiagnostic(
                 "class A { int x }\nclass Other { int x }\n"
                 "class C: A { void f() { print(Other.x) } }\nstart {}",
                 "undefined variable 'Other'");
             expectDiagnostic(
                 "class A { int x }\nclass Other { int x }\n"
                 "class C: A { void f() { print(A.Other.x) } }\nstart {}",
                 "has no field 'Other'");
             expectDiagnostic(
                 "class A { int x }\nclass B: A {}\n"
                 "class C: B { void f() { print(A.x) } }\nstart {}",
                 "undefined variable 'A'");
             expectDiagnostic(
                 "class A { int f(int n) { return n } }\n"
                 "class C: A { void g() { A.f(\"bad\") } }\nstart {}",
                 "method argument type does not match parameter 'n'");
             expectDiagnostic(
                 "class A { int x }\nclass C: A { void f() { int A = 1\n"
                 "print(A.x) } }\nstart {}",
                 "unknown class 'int'");
         }},
        {"implicit qualifiers preserve lexical class and value precedence", [] {
             expectValid(
                 "namespace N { class A { int x\n int f() { return 1 } }\n"
                 "class C: A { void g() { print(A.x)\n print(N.A.x)\n"
                 "print(A.f())\n A other = N.A()\n print(other.f())\n type t = N.A }\n"
                 "} }\nstart { N.C c = N.C() }");
             expectValid(
                 "class A { int x\n int f() { return 1 } }\n"
                 "class Object { int x\n int f() { return 2 } }\n"
                 "class C: A { Object A\n"
                 " void g(Object local) { print(A.x)\n print(A.f())\n print(local.x) }\n"
                 " void h(Object A) { print(A.x)\n print(A.f()) }\n"
                 "}\nstart {}");
             expectDiagnostic(
                 "class A { int x }\nnamespace N { class A { int x }\n"
                 "class C: A { void f() { print(N.A.x) } } }\n"
                 "namespace M { class A { int x }\n"
                 "class C: N.A { void f() { print(A.x) } } }\nstart {}",
                 "undefined variable 'A'");
             expectValid(
                 "class A { int x\n int f() { return x } }\n"
                 "class C: A { int g() }\n"
                 "int C.g() { A.x += 1\n return A.f() }\nstart {}");
         }},
        {"bare method statement reports argument count instead of parser error", [] {
             expectDiagnostic(
                 "/*\n"
                 "    Demonstrate the unknown class bug.\n"
                 "    Produces error:\n"
                 "    only_method_calls.simp:9:24: error: only method calls may be used as expression statements\n"
                 "    should be valid code.\n"
                 " */\n"
                 "class TheTest {\n"
                 "    void another_function() {\n"
                 "        some_function()\n"
                 "    }\n"
                 "    int some_function(int n) {\n"
                 "        return(n-1)\n"
                 "    }\n"
                 "}\n"
                 "start {\n"
                 "    TheTest tst = TheTest()\n"
                 "}\n",
                 "method 'some_function' argument count mismatch");
         }},
        {"bare recursive method call resolves in current class", [] {
             expectValid(
                 "/*\n"
                 "    Demonstrate the unknown class bug.\n"
                 "    Produces error:\n"
                 "    unknown_class.simp:10:20: error: unknown class 'some_function'\n"
                 " */\n"
                 "class TheTest {\n"
                 "    int some_function(int n) {\n"
                 "        if(n != 0) {\n"
                 "            return(some_function(n))\n"
                 "        }\n"
                 "        return(n-1)\n"
                 "    }\n"
                 "}\n"
                 "start {\n"
                 "    TheTest tst = TheTest()\n"
                 "    int x = 5\n"
                 "    while(x != 0) {\n"
                 "        print(tst.some_function(x))\n"
                 "    }\n"
                 "}\n");
         }},
        {"bare method calls reject wrong types and unknown methods", [] {
             expectDiagnostic(
                 "class C { void caller() { f(\"bad\") } void f(int n) {} }\nstart {}",
                 "method argument type does not match parameter 'n'");
             expectDiagnostic(
                 "class C { void caller() { missing() } }\nstart {}",
                 "class 'C' has no method 'missing'");
             expectDiagnostic(
                 "class C { int caller() { return missing() } }\nstart {}",
                 "class 'C' has no method 'missing'");
             expectDiagnostic(
                 "start { print(Missing()) }",
                 "unknown class 'Missing'");
             expectDiagnostic(
                 "class C { void f() { C() } }\nstart {}",
                 "only method calls may be used as expression statements");
         }},
        {"out-of-line method body completes an in-class declaration", [] {
             expectValid(
                 "class Foo {\n  int compute(int x)\n}\n"
                 "int Foo.compute(int x) {\n  return x * 2\n}\n"
                 "start {\n  print(Foo().compute(5))\n}");
         }},
        {"nested returns and exhaustive branches satisfy non-void methods", [] {
             expectValid(
                 "class Choice {\n"
                 "  int select(bool left) {\n"
                 "    { if (left) { return 1 } else { return 2 } }\n"
                 "  }\n"
                 "  int protectedValue() {\n"
                 "    try { return 3 } except() { return 4 } finally { print(5) }\n"
                 "  }\n"
                 "  void early() { if (true) { return } print(6) }\n"
                 "}\nstart { print(Choice().select(true)) }");
         }},
        {"non-void methods reject reachable fallthrough", [] {
             expectDiagnostic(
                 "class Choice { int select(bool left) { if (left) { return 1 } } }\n"
                 "start {}",
                 "path without a return value or raise");
             expectDiagnostic(
                 "class Choice { int select() { while (true) { return 1 } } }\n"
                 "start {}",
                 "path without a return value or raise");
             expectDiagnostic(
                 "class Choice { int select() { try { return 1 } "
                 "except() { print(0) } } }\nstart {}",
                 "path without a return value or raise");
         }},
        {"return inside finally is rejected even when nested", [] {
             expectDiagnostic(
                 "class Choice { int select() { try { return 1 } finally { "
                 "if (true) { { return 2 } } } } }\nstart {}",
                 "return is not allowed inside finally");
             expectDiagnostic(
                 "class Choice { void select() { try { print(1) } finally { "
                 "try { print(2) } except() { return } } } }\nstart {}",
                 "return is not allowed inside finally");
         }},
        {"return type and entry restrictions remain enforced", [] {
             expectDiagnostic("start { return }", "only valid inside a class method");
             expectDiagnostic("class C { void f() { if (true) { return 1 } } }\nstart {}",
                              "void method cannot return a value");
             expectDiagnostic("class C { int f() { return } }\nstart {}",
                              "non-void method must return a value");
             expectDiagnostic("class C { int f() { { return \"bad\" } } }\nstart {}",
                              "return type does not match method return type");
             expectDiagnostic("class C { C() { return 1 } }\nstart {}",
                              "void method cannot return a value");
             expectDiagnostic("class C { destroy { return 1 } }\nstart {}",
                              "void method cannot return a value");
             expectValid("class C { C() { return } destroy { return } }\n"
                         "start { C value = C() }");
             expectDiagnostic("class C { void destroy() {} }\nstart {}",
                              "destructors use 'destroy { ... }' syntax");
             expectDiagnostic("class C { destroy(int value) {} }\nstart {}",
                              "destructors use 'destroy { ... }' syntax");
             expectDiagnostic("class C { int destroy() {} }\nstart {}",
                              "destructors use 'destroy { ... }' syntax");
         }},
        {"external method validates and calls through ordinary method syntax", [] {
             expectValid(
                 "class Native {\n  int absolute(int value)\n}\n"
                 "int Native.absolute(int value) from \"simp_method_demo_abs\"\n"
                 "start {\n  print(Native().absolute(5))\n}");
         }},
        {"external method argument count is validated", [] {
             expectDiagnostic(
                 "class Native {\n  int absolute(int value)\n}\n"
                 "int Native.absolute(int value) from \"simp_method_demo_abs\"\n"
                 "start {\n  print(Native().absolute(1, 2))\n}",
                 "method 'absolute' argument count mismatch");
         }},
        {"external method argument type is validated", [] {
             expectDiagnostic(
                 "class Native {\n  int absolute(int value)\n}\n"
                 "int Native.absolute(int value) from \"simp_method_demo_abs\"\n"
                 "start {\n  strg text = \"x\"\n  print(Native().absolute(text))\n}",
                 "method argument type does not match parameter 'value'");
         }},
        {"external method rejects 'any' parameters", [] {
             expectDiagnostic(
                 "class Native {\n  int use(any value)\n}\n"
                 "int Native.use(any value) from \"symbol\"\n"
                 "start {\n  print(1)\n}",
                 "'any' cannot be used as a declared type");
         }},
        {"external method rejects 'any' returns", [] {
             expectDiagnostic(
                 "class Native {\n  any dynamicValue(int value)\n}\n"
                 "any Native.dynamicValue(int value) from \"symbol\"\n"
                 "start {\n  print(1)\n}",
                 "'any' cannot be used as a declared type");
         }},
        {"external method requires a non-empty C symbol", [] {
             expectDiagnostic(
                 "class Native {\n  int value()\n}\n"
                 "int Native.value() from \"\"\n"
                 "start {\n  print(1)\n}",
                 "native-bound method must name a non-empty C symbol");
         }},
        {"out-of-line method signature must match its declaration", [] {
             expectDiagnostic(
                 "class Native {\n  int absolute(int value)\n}\n"
                 "strg Native.absolute(strg value) from \"symbol\"\n"
                 "start {\n  print(1)\n}",
                 "out-of-line definition signature does not match declaration");
         }},
        {"out-of-line method definition must be unique", [] {
             expectDiagnostic(
                 "class Native {\n  int absolute(int value)\n}\n"
                 "int Native.absolute(int value) from \"symbol\"\n"
                 "int Native.absolute(int value) from \"symbol\"\n"
                 "start {\n  print(1)\n}",
                 "duplicate out-of-line definition for method 'Native.absolute'");
         }},
        {"out-of-line methods require an in-class declaration", [] {
             expectDiagnostic(
                 "class Native {\n}\n"
                 "int Native.absolute(int value) from \"symbol\"\n"
                 "start {\n  print(1)\n}",
                 "has no in-class declaration for method 'absolute'");
         }},
        {"bodyless method declarations require a definition", [] {
             expectDiagnostic(
                 "class Native {\n  int absolute(int value)\n}\n"
                 "start {\n  print(1)\n}",
                 "is declared but has no out-of-line definition");
         }},
        {"external symbols require ABI-compatible method signatures", [] {
             expectDiagnostic(
                 "class Native {\n"
                 "  int integerValue(int value)\n"
                 "  strg stringValue(strg value)\n"
                 "}\n"
                 "int Native.integerValue(int value) from \"same_symbol\"\n"
                 "strg Native.stringValue(strg value) from \"same_symbol\"\n"
                 "start {\n  print(1)\n}",
                 "is reused with an incompatible method signature");
         }},
        {"legacy top-level extern declaration syntax is rejected", [] {
             expectDiagnostic(
                 "extern int c_abs(int value) from \"abs\"\n"
                 "start {\n  print(1)\n}",
                 "program must contain exactly one top-level 'start' block");
         }}
}};

} // namespace
