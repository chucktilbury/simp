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

const TestGroupRegistration registration{4, {
        {"array literals accept mixed int, string, and class values", [] {
             expectValid(
                 "class Widget {\n  int id\n  Widget(int initial) { id = initial }\n}\n"
                 "start {\n"
                 "  array bag = [1, \"two\", Widget(3), null]\n"
                 "  print(bag.length)\n"
                 "}");
         }},
        {"'list' is an alias for the 'array' type", [] {
             expectValid("start {\n"
                         "  list values = [1, \"two\"]\n"
                         "  array alias = values\n"
                         "  print(alias.length)\n"
                         "}");
         }},
        {"array element reads yield 'any' and can be extracted by type", [] {
             expectValid("start {\n"
                         "  array bag = [1, \"two\"]\n"
                         "  any first = bag[0]\n"
                         "  int extracted = first\n"
                         "  string second = bag[1]\n"
                         "  print(extracted)\n"
                         "  print(second)\n"
                         "}");
         }},
        {"array element assignment accepts any supported value type", [] {
             expectValid("start {\n"
                         "  array bag = [1, \"two\"]\n"
                         "  bag[0] = \"now a string\"\n"
                         "  bag[1] = 42\n"
                         "  print(bag.length)\n"
                         "}");
         }},
        {"legacy 'int[]' array syntax is rejected", [] {
             expectDiagnostic("start {\n  int[] values = [1, 2]\n}",
                              "expected identifier (keywords are reserved)");
         }},
        {"legacy 'string[]' array syntax is rejected", [] {
             expectDiagnostic("start {\n  string[] values = [\"a\"]\n}",
                              "expected identifier (keywords are reserved)");
         }},
        {"legacy 'Class[]' array syntax is rejected", [] {
             expectDiagnostic(
                 "class Widget {\n  int id\n  Widget(int initial) { id = initial }\n}\n"
                 "start {\n  Widget[] values = [Widget(1)]\n}",
                 "expected expression");
         }},
        {"nested array literals are accepted", [] {
             expectValid("start {\n  array inner = [2, 3]\n"
                         "  array bag = [1, inner, [4, 5]]\n}");
         }},
        {"'any' has no members until extracted", [] {
             expectDiagnostic("start {\n  array bag = [1]\n  any first = bag[0]\n"
                              "  print(first.length)\n}",
                              "'any' has no members; assign it to a typed variable first to "
                              "extract its value");
         }},
        {"array and 'any' equality is rejected", [] {
             expectDiagnostic("start {\n  array bag = [1]\n  array other = [1]\n"
                              "  int same = bag == other\n}",
                              "equality requires matching int, bool, float, or unsigned operands");
         }},
        {"map type and literal keys are accepted", [] {
             expectValid("start {\n"
                         "  dict values = {\"answer\": 42, 'label': \"ok\"}\n"
                         "  map alias = values\n"
                         "  any result = alias[\"answer\"]\n"
                         "  alias[\"answer\"] = \"changed\"\n"
                         "  print(alias.length)\n"
                         "}");
         }},
        {"map indexing accepts string expressions", [] {
             expectValid("start {\n map values = {}\n string key = \"x\"\n"
                         " any value = values[key]\n}");
         }},
        {"map literal keys must be string literals", [] {
             expectDiagnostic("start { map values = {1: \"value\"} }",
                              "map keys must have type string");
         }},
        {"map length is read-only", [] {
             expectDiagnostic("start {\n map values = {}\n values.length = 1\n}",
                              "map length is read-only");
         }},
        {"map equality is rejected", [] {
             expectDiagnostic("start {\n map values = {}\n map other = {}\n"
                              " int same = values == other\n}",
                              "equality requires matching int, bool, float, or unsigned operands");
         }},
        {"'any' equality is rejected", [] {
             expectDiagnostic("start {\n any value = 1\n"
                              " int same = value == value\n}",
                              "equality requires matching int, bool, float, or unsigned operands");
         }},
        {"map slicing accepts integer insertion-order bounds", [] {
             expectValid("start {\n map values = {\"a\": 1, \"b\": 2}\n"
                         " map copy = values[0:1]\n"
                         " print(copy.length)\n}");
         }},
        {"map removal accepts one string key", [] {
             expectValid("start {\n map values = {\"a\": 1}\n"
                         " int removed = values.remove(\"a\")\n"
                         " print(removed)\n}");
         }},
        {"map removal rejects a non-string key", [] {
             expectDiagnostic("start {\n map values = {}\n"
                              " int removed = values.remove(1)\n}",
                              "map key must have type string");
         }},
        {"map removal rejects incorrect arity", [] {
             expectDiagnostic("start {\n map values = {}\n"
                              " int removed = values.remove()\n}",
                              "map 'remove' expects one string key");
         }},
        {"map slice bounds require integers", [] {
             expectDiagnostic("start {\n map values = {}\n"
                              " map copy = values[\"a\":\"z\"]\n}",
                              "map index and slice bounds must be int");
         }},
         {"map contains and collection iteration are accepted", [] {
              expectValid("start {\n"
                          "  map values = {\"one\": 1}\n"
                          "  string key = \"one\"\n"
                          "  int present = values.contains(key)\n"
                          "  for (name, value in values) { print(name) }\n"
                          "  array items = [1, \"two\"]\n"
                          "  for (item in items) { print(item) }\n"
                          "}");
         }},
         {"map iteration supports value-only and key/value bindings", [] {
              expectValid("start {\n"
                          "  map values = {\"one\": 1}\n"
                          "  for (value in values) { print(value) }\n"
                          "  for (key, value in values) { print(key) }\n"
                          "}");
         }},
         {"array iteration accepts one value variable", [] {
              expectDiagnostic("start {\n array values = []\n"
                               " for (key, value in values) { print(value) }\n}",
                               "array iteration accepts one value variable");
         }},
         {"map keys must be statically typed strings", [] {
              expectDiagnostic("start {\n map values = {}\n any key = \"x\"\n"
                               " any value = values[key]\n}",
                               "map keys must have type string");
         }}
}};

} // namespace
