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

const TestGroupRegistration registration{1, {
        {"top-level import syntax is represented in the AST", [] {
             simp::Lexer lexer("import network as Net\nstart {}", "import.simp");
             auto program = simp::Parser(lexer.tokenize()).parseProgram();
             require(program.imports.size() == 1 &&
                         program.imports.front().moduleName == "network" &&
                         program.imports.front().alias == "Net",
                     "import module name or alias missing from AST");
         }},
        {"import requires an alias", [] {
             expectDiagnostic("import network\nstart {}", "expected 'as' after module name");
         }},
        {"import is rejected in declaration and function bodies", [] {
             expectDiagnostic("namespace Hidden { import network as Net }\nstart {}",
                              "'import' is only allowed at top level");
             expectDiagnostic("class Hidden { import network as Net }\nstart {}",
                              "'import' is only allowed at top level");
             expectDiagnostic("start { import network as Net }",
                              "'import' is only allowed at top level");
         }},
        {"create is no longer a reserved keyword", [] {
             simp::Lexer lexer("start {\n int create = 1\n print(create)\n}", "identifier.simp");
             const auto tokens = lexer.tokenize();
             require(tokens[4].type == simp::TokenType::Identifier &&
                         tokens[4].text == "create",
                     "create should be lexed as an identifier");
         }},
        {"strg is the primitive string type and string remains an identifier", [] {
             expectValid("class string { string() {} }\n"
                         "class Text { strg echo(strg value) { return value } }\n"
                         "start {\n strg text = Text().echo(\"value\")\n"
                         " int string = 1\n string custom = string()\n print(string)\n}");
             expectDiagnostic("start {\n string text = \"value\"\n}",
                              "unknown type or class 'string'");
         }},
        {"class-name constructor syntax", [] {
             const auto program = parse(
                 "class Box { Box() {} }\nstart { Box box = Box() }");
             std::ostringstream output;
             simp::dumpAst(program, output);
             require(output.str().find("ConstructorCall [Box]") != std::string::npos,
                     "class-name constructor call missing from AST");
         }},
        {"single inheritance and base constructor syntax", [] {
             const auto program = parse(
                 "class Base { Base(int value) {} }\n"
                 "class Child : Base { Child(int value) { super Base(value) } }\n"
                 "start { Child child = Child(1) }");
             std::ostringstream output;
             simp::dumpAst(program, output);
             require(output.str().find("Class [Child : public Base]") != std::string::npos,
                     "base class missing from AST");
             require(output.str().find("Super constructor [Base]") != std::string::npos,
                     "explicit base constructor missing from AST");
         }},
        {"virtual base initializer syntax", [] {
             const auto program = parse(
                 "class Root { Root(int value) {} }\n"
                 "class Leaf : virtual Root {\n"
                 "  Leaf(int value) { virtual super Root(value) }\n"
                 "}\n"
                 "start { Leaf leaf = Leaf(1) }");
             std::ostringstream output;
             simp::dumpAst(program, output);
             require(output.str().find("Super virtual constructor [Root]") !=
                         std::string::npos,
                     "virtual base initializer missing from AST");
         }},
        {"dotless base initializer modifier orders", [] {
             const auto program = parse(
                 "class Root { Root(int value) {} }\n"
                 "class First : virtual Root {\n"
                 "  First(int value) { super virtual Root(value) }\n"
                 "}\n"
                 "class Second : virtual Root {\n"
                 "  Second(int value) { virtual super Root(value) }\n"
                 "}\n"
                 "start { First first = First(1)\n Second second = Second(2) }");
             std::ostringstream output;
             simp::dumpAst(program, output);
             const auto tree = output.str();
             const auto first = tree.find("Super virtual constructor [Root]");
             require(first != std::string::npos &&
                         tree.find("Super virtual constructor [Root]", first + 1) !=
                             std::string::npos,
                     "both virtual initializer modifier orders should appear in the AST");
         }},
        {"dotted base initializer syntax is rejected", [] {
             expectDiagnostic(
                 "class Base {}\n"
                 "class Child : Base { Child() { super.Base() } }\n"
                 "start {}",
                 "base class name after super");
             expectDiagnostic(
                 "class Root {}\n"
                 "class Leaf : virtual Root { Leaf() { super.virtual Root() } }\n"
                 "start {}",
                 "base class name after super");
         }},
        {"malformed virtual base initializer modifier order is rejected", [] {
             expectDiagnostic(
                 "class Root {}\n"
                 "class Leaf : virtual Root { Leaf() { virtual Root() } }\n"
                 "start {}",
                 "'super' after virtual");
         }},
        {"virtual base syntax is retained in AST", [] {
             const auto program = parse(
                 "class Root {}\n"
                 "class Left : public virtual Root {}\n"
                 "class Other : virtual public Root {}\n"
                 "start {}");
             std::ostringstream output;
             simp::dumpAst(program, output);
             require(output.str().find("Class [Left : public virtual Root]") !=
                         std::string::npos,
                     "virtual base modifier missing from AST");
             require(output.str().find("Class [Other : public virtual Root]") !=
                         std::string::npos,
                     "virtual/access modifier ordering was not accepted");
         }},
        {"parser precedence and AST", [] {
             const auto program = parse("start {\n int x = 1 + 2 * 3\n print(x)\n}");
             require(program.statements.size() == 2, "expected declaration and print");
             std::ostringstream output;
             simp::dumpAst(program, output);
             const auto tree = output.str();
             require(tree.find("Binary [+]") != std::string::npos, "addition absent from AST");
             require(tree.find("Binary [*]") != std::string::npos, "multiplication absent from AST");
         }},
        {"compound assignments are assignment statements in the AST", [] {
             simp::Lexer lexer(
                 "start {\n int value = 12\n value += 1\n value -= 1\n value *= 2\n"
                 " value /= 2\n value %= 3\n}",
                 "compound-assignment.simp");
             const auto program = simp::Parser(lexer.tokenize()).parseProgram();
             require(program.statements.size() == 6,
                     "compound assignments did not parse as separate statements");
             std::ostringstream output;
             simp::dumpAst(program, output);
             for (const auto* operation : {"+=", "-=", "*=", "/=", "%="}) {
                 require(output.str().find("Assignment [" + std::string(operation) + "]") !=
                             std::string::npos,
                         "compound assignment operator missing from AST");
             }
         }},
        {"dict literals are represented in the AST", [] {
             const auto program = parse(
                 "start {\n dict values = {\"answer\": 42}\n}");
             std::ostringstream output;
             simp::dumpAst(program, output);
             require(output.str().find("MapLiteral") != std::string::npos,
                     "dict literal missing from AST");
         }},
        {"raise, catch-all, and finally syntax", [] {
             const auto program = parse(
                 "class Failure : Exception {\n"
                 "  Failure(strg text) { super Exception(text) }\n"
                 "}\n"
                 "start {\n"
                 "  try { raise(Failure(\"failure\")) } except() { print(\"caught\") } "
                 "finally { print(\"done\") }\n"
                 "}");
             std::ostringstream output;
             simp::dumpAst(program, output);
             require(output.str().find("Try") != std::string::npos &&
                         output.str().find("Raise") != std::string::npos &&
                         output.str().find("Except") != std::string::npos &&
                         output.str().find("Finally") != std::string::npos,
                     "exception constructs missing from AST");
         }},
        {"except may bind a read-only exception string", [] {
             const auto program = parse(
                 "start {\n try { raise(Exception(\"message\")) } "
                 "except() as error { print(error) }\n}");
             const auto& handlers = program.statements.front().exceptionHandlers;
             require(handlers.size() == 1 && handlers.front().hasBinding &&
                         handlers.front().name == "error",
                     "except binding missing from AST");
         }},
        {"try supports ordered typed exception clauses", [] {
             const auto program = parse(
                 "class Parent : Exception { Parent(strg text) { super Exception(text) } }\n"
                 "class Child : Parent { Child(strg text) { super Parent(text) } }\n"
                 "start {\n"
                 " try { raise(Child(\"message\")) }\n"
                 " except(Parent) as parent { print(parent.message) }\n"
                 " except() { print(\"fallback\") }\n"
                 " finally { print(\"done\") }\n"
                 "}");
             const auto& handlers = program.statements.front().exceptionHandlers;
             require(handlers.size() == 2 && handlers[0].exceptionType == "Parent" &&
                         handlers[0].hasBinding && handlers[1].exceptionType.empty(),
                     "ordered except clauses missing from AST");
         }},
        {"catch-all exception clause must be last", [] {
             expectDiagnostic(
                 "start {\n try { print(1) }\n"
                 " except() { print(2) }\n"
                 " except(Exception) { print(3) }\n}",
                 "catch-all except clause must be last");
         }},
        {"duplicate catch-all exception clauses are rejected", [] {
             expectDiagnostic(
                 "start {\n try { print(1) }\n"
                 " except() { print(2) }\n"
                 " except() { print(3) }\n}",
                 "duplicate catch-all except clause");
         }},
        {"subclass exception clause after base is unreachable", [] {
             expectDiagnostic(
                 "class Parent : Exception { Parent(strg text) { super Exception(text) } }\n"
                 "class Child : Parent { Child(strg text) { super Parent(text) } }\n"
                 "start {\n try { print(1) }\n"
                 " except(Parent) { print(2) }\n"
                 " except(Child) { print(3) }\n}",
                 "unreachable after earlier 'Parent' clause");
         }},
        {"catch-all after Exception clause is unreachable", [] {
             expectDiagnostic(
                 "start {\n try { print(1) }\n"
                 " except(Exception) { print(2) }\n"
                 " except() { print(3) }\n}",
                 "catch-all except clause is unreachable after 'Exception'");
         }},
        {"try requires a handler or finally", [] {
             expectDiagnostic("start { try { print(1) } }",
                              "requires an 'except' or 'finally' block");
         }},
        {"except requires parenthesized filter", [] {
             expectDiagnostic("start { try { print(1) } except { print(2) } }",
                              "'(' after except");
             expectDiagnostic("start { try { print(1) } except as error { print(error) } }",
                              "'(' after except");
             expectDiagnostic("start { try { print(1) } except Exception { print(2) } }",
                              "'(' after except");
             expectDiagnostic("start { try { print(1) } except Exception as error { print(error.message) } }",
                              "'(' after except");
             expectDiagnostic("start { try { print(1) } except(Exception { print(2) } }",
                              "')' after exception class name");
         }},
        {"except accepts qualified class paths", [] {
             const auto program = parse(
                 "namespace errors {\n"
                 " class Failure : Exception { Failure(strg text) { super Exception(text) } }\n"
                 "}\n"
                 "start { try { raise(errors.Failure(\"message\")) } "
                 "except(errors.Failure) as caught { print(caught.message) } }\n");
             const auto& handlers = program.statements.front().exceptionHandlers;
             require(handlers.size() == 1 &&
                         handlers.front().exceptionType == "errors.Failure" &&
                         handlers.front().hasBinding,
                     "qualified exception filter missing from AST");
         }},
        {"raise requires a concrete exception constructor", [] {
             expectDiagnostic("start { raise(\"message\") }",
                              "raise requires a constructor expression");
             expectDiagnostic("start { raise(null) }",
                              "raise requires a constructor expression");
        }},
        {"raise without an expression requires an active handler", [] {
             expectDiagnostic("start { raise() }",
                              "raise() is only valid inside an except handler");
         }},
        {"newline statement boundaries", [] {
             const auto program = parse(
                 "class Base {\n"
                 "  int value\n"
                 "  Base(int initial) {\n"
                 "    value = initial\n"
                 "  }\n"
                 "}\n"
                 "class Child : Base {\n"
                 "  Child(int initial) {\n"
                 "    super Base(initial)\n"
                 "  }\n"
                 "  int read() {\n"
                 "    return value\n"
                 "  }\n"
                 "}\n"
                 "start {\n"
                 "  Child child = Child(40)\n"
                 "  child.value = child.read() + 2\n"
                 "  print(child.value)\n"
                 "}");
             require(program.statements.size() == 3,
                     "expected semicolon-free declarations, assignment, and print");
         }},
        {"adjacent statements without a boundary are rejected", [] {
             expectDiagnostic("start {\n int first = 1 int second = 2\n}",
                              "expected newline after statement");
         }},
        {"parser trace", [] {
             simp::Lexer lexer("start {}", "trace.simp");
             std::ostringstream trace;
             simp::Parser parser(lexer.tokenize(), &trace);
             parser.parseProgram();
             require(trace.str().find("[parser]") != std::string::npos, "parser trace was empty");
         }}
}};

} // namespace
