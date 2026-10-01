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

const TestGroupRegistration registration{0, {
        {"lexer keywords and strings", [] {
             simp::Lexer lexer("StArT {\n INT n = 12\n StRg s = \"a\\n\"\n}", "lexer.simp");
             const auto tokens = lexer.tokenize();
             require(tokens[0].type == simp::TokenType::Start, "START keyword not recognized");
             require(tokens[3].type == simp::TokenType::Int, "INT keyword not recognized");
             require(tokens[5].type == simp::TokenType::Equal, "assignment token missing");
             require(tokens[6].type == simp::TokenType::Integer && tokens[6].text == "12",
                     "integer token incorrect");
             require(tokens[8].type == simp::TokenType::StrgType,
                     "STRG type keyword not recognized");
             require(tokens[11].type == simp::TokenType::String &&
                         tokens[11].text == "a\n" && tokens[11].formattedString,
                     "escaped formatted string incorrect");
         }},
        {"single-quoted strings are literal", [] {
             simp::Lexer lexer("start {\n print('a\\n')\n}", "single-quoted.simp");
             const auto tokens = lexer.tokenize();
             require(tokens[5].type == simp::TokenType::String, "single-quoted token missing");
             require(tokens[5].text == "a\\n", "single-quoted backslash was interpreted");
             require(!tokens[5].formattedString, "single-quoted token was marked formatted");
         }},
        {"double-quoted escape e produces ESC only", [] {
             simp::Lexer lexer("start { print(\"a\\e\") print('a\\e') }",
                               "escape-e.simp");
             const auto tokens = lexer.tokenize();
             const auto expected = std::string("a") + static_cast<char>(0x1b);
             require(tokens[4].type == simp::TokenType::String &&
                         tokens[4].text == expected,
                     "double-quoted \\\\e did not produce byte 0x1b");
             require(tokens[8].type == simp::TokenType::String &&
                         tokens[8].text == "a\\e",
                     "single-quoted \\\\e did not preserve its backslash");
         }},
        {"semicolon line comments", [] {
             simp::Lexer lexer("start {\n print(1) ; comment to newline\n print(2)\n}",
                               "semicolon-comment.simp");
             const auto tokens = lexer.tokenize();
             std::size_t integers = 0;
             for (const auto& token : tokens) {
                 if (token.type == simp::TokenType::Integer) ++integers;
             }
             require(integers == 2, "semicolon comment swallowed the following line");
         }},
        {"hash line comments", [] {
             simp::Lexer lexer("start {\n # comment\n print(2)\n}", "hash-comment.simp");
             const auto tokens = lexer.tokenize();
             bool foundPrint = false;
             for (const auto& token : tokens) foundPrint |= token.type == simp::TokenType::Print;
             require(foundPrint,
                     "hash comment did not preserve the following line");
         }},
        {"slash line comments", [] {
             simp::Lexer lexer("start {\n // comment\n print(2)\n}", "slash-comment.simp");
             const auto tokens = lexer.tokenize();
             bool foundPrint = false;
             for (const auto& token : tokens) foundPrint |= token.type == simp::TokenType::Print;
             require(foundPrint,
                     "slash comment did not preserve the following line");
         }},
        {"block comments preserve statement boundaries", [] {
             simp::Lexer lexer("start {\n int value = 1 /* comment\ncontinued */\n print(value)\n}",
                               "block-comment.simp");
             const auto tokens = lexer.tokenize();
             std::size_t newlines = 0;
             for (const auto& token : tokens) {
                 if (token.type == simp::TokenType::Newline) ++newlines;
             }
             require(newlines >= 3, "block comment swallowed statement-boundary newlines");
         }},
        {"unterminated block comment diagnostic", [] {
             expectDiagnostic("start {\n /* comment", "unterminated block comment");
         }},
        {"reserved keywords", [] {
             expectDiagnostic("start {\n int While = 0\n}", "keywords are reserved");
         }},
         {"old string spelling is an identifier", [] {
              simp::Lexer lexer("start { int string = 1 }", "string-identifier.simp");
              const auto tokens = lexer.tokenize();
              require(tokens[3].type == simp::TokenType::Identifier &&
                          tokens[3].text == "string",
                      "lowercase string was treated as a keyword");
          }},
        {"scalar keywords and literals", [] {
             simp::Lexer lexer("start { bool ready = true float value = 3.14 "
                               "unsigned count = 42u }", "scalar.simp");
             const auto tokens = lexer.tokenize();
             bool foundTrue = false;
             bool foundFloat = false;
             bool foundUnsigned = false;
             for (const auto& token : tokens) {
                 foundTrue |= token.type == simp::TokenType::True;
                 foundFloat |= token.type == simp::TokenType::Float && token.text == "3.14";
                 foundUnsigned |= token.type == simp::TokenType::UnsignedInteger &&
                                  token.text == "42u";
             }
             require(foundTrue && foundFloat && foundUnsigned,
                     "boolean, float, or unsigned literal token missing");
         }},
        {"import keyword is case-insensitive and reserved", [] {
             simp::Lexer lexer("IMPORT network AS Net\nstart {}", "import-keyword.simp");
             const auto tokens = lexer.tokenize();
             require(tokens[0].type == simp::TokenType::Import &&
                         tokens[2].type == simp::TokenType::As,
                     "import keywords were not recognized case-insensitively");
         }}
}};

} // namespace
