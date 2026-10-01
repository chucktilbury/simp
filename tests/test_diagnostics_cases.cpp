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

const TestGroupRegistration registration{2, {
        {"diagnostic source location", [] {
             try {
                 simp::Lexer lexer("start {\n @\n}", "location.simp");
                 lexer.tokenize();
             } catch (const simp::DiagnosticError& error) {
                 require(std::string(error.what()).find("location.simp:2:2: error:") == 0,
                         "diagnostic location was incorrect");
                 return;
             }
             throw std::runtime_error("invalid character did not produce a diagnostic");
         }},
        {"invalid UTF-8 string rejected", [] {
             std::string source = "start { print(\"";
             source.push_back(static_cast<char>(0xc0));
             source += "\"); }";
             try {
                 simp::Lexer lexer(source, "invalid-utf8.simp");
                 lexer.tokenize();
             } catch (const simp::DiagnosticError& error) {
                 require(std::string(error.what()).find("string literal is not valid UTF-8") !=
                             std::string::npos,
                         "invalid UTF-8 diagnostic was missing");
                 return;
             }
             throw std::runtime_error("invalid UTF-8 string was accepted");
         }},
        {"formatted print arity", [] {
             expectDiagnostic("start {\n print(\"value: {}\"(1, 2))\n}",
                              "one '{}' placeholder per argument");
         }},
        {"formatted print malformed brace", [] {
             expectDiagnostic("start {\n print(\"value: {x}\"(1))\n}",
                              "only '{}' placeholders are supported");
         }},
        {"single-quoted string format arguments", [] {
             expectDiagnostic("start {\n print('value: {}'(1))\n}",
                              "format arguments require a double-quoted string literal");
         }},
        {"string condition rejected", [] {
             expectDiagnostic("start { if (\"not a condition\") { } }",
                              "if condition must have type bool");
         }},
        {"string ordering rejected", [] {
             expectDiagnostic("start {\n strg a = \"a\"\n print(a < \"a\")\n}",
                              "requires matching int, float, or unsigned operands");
         }},
        {"integer condition rejected", [] {
             expectDiagnostic("start { if (1) { } }", "if condition must have type bool");
         }},
        {"semantic type mismatch", [] {
             expectDiagnostic("start {\n int value = \"wrong\"\n}",
                              "cannot initialize int variable with String");
         }},
        {"semantic assignment type mismatch", [] {
             expectDiagnostic("start {\n int value = 1\n value = \"wrong\"\n}",
                              "cannot assign String to int variable 'value'");
         }},
        {"semantic undefined variable", [] {
             expectDiagnostic("start {\n print(missing)\n}", "undefined variable 'missing'");
         }},
        {"semantic definite initialization", [] {
             expectDiagnostic("start {\n int value\n print(value)\n}",
                              "variable 'value' may be uninitialized");
         }},
        {"semantic branch initialization", [] {
             expectDiagnostic("start {\n int value\n if (true) { value = 1 }\n print(value)\n}",
                              "variable 'value' may be uninitialized");
         }},
        {"semantic initialized in both branches", [] {
             const auto program = parse(
                 "start {\n int value\n if (true) { value = 1 } else { value = 2 }\n print(value)\n}");
             require(program.statements.size() == 3, "expected declaration, if, and print");
         }},
        {"virtual-base initializer rejected outside constructors", [] {
             expectDiagnostic(
                 "class Root { Root(int value) {} }\n"
                 "class Leaf : virtual Root {\n"
                 "  void method() { super.virtual Root(1) }\n"
                 "}\n"
                 "start {}",
                 "super initializers must be direct leading constructor statements");
         }}
}};

} // namespace
