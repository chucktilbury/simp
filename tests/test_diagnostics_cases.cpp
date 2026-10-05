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
             expectDiagnostic("start {\n print(format(\"value: {}\", 1, 2))\n}",
                              "one '{}' placeholder per argument");
         }},
        {"formatted print malformed brace", [] {
             expectDiagnostic("start {\n print(format(\"value: {x-y}\", x=1))\n}",
                              "malformed placeholder name");
         }},
        {"formatted string unmatched braces", [] {
             expectDiagnostic("start {\n print(format(\"value: {name\", name=1))\n}",
                              "unmatched '{'");
             expectDiagnostic("start {\n print(format(\"value: name}\", name=1))\n}",
                              "unmatched '}'");
         }},
        {"formatted string rejects mixed placeholder and argument forms", [] {
             expectDiagnostic("start {\n print(format(\"{} {name}\", name=1))\n}",
                              "named and positional placeholders cannot be mixed");
             expectDiagnostic("start {\n print(format(\"{name}\", 1))\n}",
                              "named and positional format arguments cannot be mixed");
             expectDiagnostic("start {\n print(format(\"{}\", name=1))\n}",
                              "named and positional format arguments cannot be mixed");
             expectDiagnostic("start {\n print(format(\"{name}\", name=1, 2))\n}",
                              "named and positional format arguments cannot be mixed");
         }},
        {"formatted string named argument validation", [] {
             expectDiagnostic("start {\n print(format(\"{name}\"))\n}",
                              "missing named format argument 'name'");
             expectDiagnostic("start {\n print(format(\"{name}\", name=1, name=2))\n}",
                              "duplicate named format argument 'name'");
             expectDiagnostic("start {\n print(format(\"{name}\", other=1))\n}",
                              "unused named format argument 'other'");
             expectDiagnostic("start {\n print(format(\"plain\", other=1))\n}",
                              "unused named format argument 'other'");
         }},
        {"named formatting preserves non-printable value rejection", [] {
             expectDiagnostic("start {\n buffer bytes = buffer(1)\n"
                              " print(format(\"{value}\", value=bytes))\n}",
                              "formatted string argument is not printable");
         }},
        {"formatted string placeholder diagnostics include source location", [] {
             try {
                 (void)parse("start {\n print(format(\"{bad-name}\", bad=1))\n}", "format.simp");
             } catch (const simp::DiagnosticError& error) {
                 require(std::string(error.what()).find("format.simp:2:8: error:") == 0,
                         "malformed format placeholder diagnostic did not identify its source");
                 return;
             }
             throw std::runtime_error("malformed format placeholder was accepted");
         }},
        {"single-quoted string format arguments", [] {
             expectDiagnostic("start {\n print('value: {}'(1))\n}",
                              "string-literal calls were removed");
         }},
        {"double-quoted string calls removed", [] {
             expectDiagnostic("start {\n print(\"{}\"(1))\n}",
                              "string-literal calls were removed");
         }},
        {"format templates must be literals", [] {
             expectDiagnostic("start {\n strg text = \"{}\"\n print(format(text, 1))\n}",
                              "format requires a string literal template");
             expectDiagnostic("start {\n strg text = \"{}\"\n print(text, 1)\n}",
                              "formatted print requires a string literal template");
         }},
        {"format specifier validation", [] {
             for (const auto& spec : {"q", "8xx", ".2f", "0>8", "<08", "08c",
                                      "1000001", "999999999999999999999"}) {
                 expectDiagnostic("start {\n print(\"{:" + std::string(spec) + "}\", 1)\n}",
                                  spec == std::string("1000001") || spec[0] == '9'
                                      ? "format width exceeds" : "invalid format specifier");
             }
             for (const auto& spec : {"d", "x", "X", "c", "08"}) {
                 expectDiagnostic("start {\n print(\"{:" + std::string(spec) + "}\", true)\n}",
                                  "numeric format specifiers require int or unsigned");
             }
             expectDiagnostic("start {\n print(\"{}\", 1, 2)\n}",
                              "one '{}' placeholder per argument");
             expectDiagnostic("start {\n print(\"{name}\", name=1, name=2)\n}",
                              "duplicate named format argument");
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
        {"compound assignments require matching arithmetic types", [] {
             expectDiagnostic("start {\n int value = 1\n value += 2u\n}",
                              "operator '+=' requires matching");
             expectDiagnostic("start {\n float value = 1.0\n value %= 2.0\n}",
                              "float remainder is unsupported");
         }},
        {"compound assignments reject unsupported targets", [] {
             expectDiagnostic("start {\n bool ready = true\n ready += true\n}",
                              "requires an int, unsigned, or float target");
             expectDiagnostic("start {\n list values = [1]\n values[0] += 1\n}",
                              "target must be a scalar variable or object field");
             expectDiagnostic("class Item {}\nstart {\n Item value = Item()\n"
                              " value += value\n}",
                              "requires an int, unsigned, or float target");
         }},
        {"compound assignments require initialized values", [] {
             expectDiagnostic("start {\n int value\n value += 1\n}",
                              "variable 'value' may be uninitialized");
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
                 "  void method() { super virtual Root(1) }\n"
                 "}\n"
                 "start {}",
                 "super initializers must be direct leading constructor statements");
         }}
}};

} // namespace
