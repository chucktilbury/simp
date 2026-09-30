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
        {"out-of-line method body completes an in-class declaration", [] {
             expectValid(
                 "class Foo {\n  int compute(int x)\n}\n"
                 "int Foo.compute(int x) {\n  return x * 2\n}\n"
                 "start {\n  print(Foo().compute(5))\n}");
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
                 "start {\n  string text = \"x\"\n  print(Native().absolute(text))\n}",
                 "method argument type does not match parameter 'value'");
         }},
        {"external method rejects 'any' parameters", [] {
             expectDiagnostic(
                 "class Native {\n  int use(any value)\n}\n"
                 "int Native.use(any value) from \"symbol\"\n"
                 "start {\n  print(1)\n}",
                 "'any' is not supported in external method signatures");
         }},
        {"external method rejects 'any' returns", [] {
             expectDiagnostic(
                 "class Native {\n  any dynamicValue(int value)\n}\n"
                 "any Native.dynamicValue(int value) from \"symbol\"\n"
                 "start {\n  print(1)\n}",
                 "'any' is not supported in external method signatures");
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
                 "string Native.absolute(string value) from \"symbol\"\n"
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
                 "  string stringValue(string value)\n"
                 "}\n"
                 "int Native.integerValue(int value) from \"same_symbol\"\n"
                 "string Native.stringValue(string value) from \"same_symbol\"\n"
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
