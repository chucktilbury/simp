#include "test_cases.hpp"

#include "simp/CodeGenerator.hpp"

#include <regex>
#include <string>

namespace {
using namespace simp_test;

const TestGroupRegistration registration{2, {
    {"all scalar cast pairs and bool type values", [] {
         const std::pair<std::string, std::string> scalars[] = {
             {"bool", "true"}, {"int", "-7"}, {"unsigned", "12u"}, {"float", "3.75"}
         };
         for (const auto& source : scalars) {
             for (const auto& target : scalars) {
                 expectValid("start {\n " + source.first + " value = " + source.second +
                             "\n " + target.first + " converted = " + target.first +
                             "(value)\n print(converted)\n}");
             }
         }
         expectValid("start {\n print(bool)\n print(true is bool)\n print(bool(1))\n}");
     }},
    {"non-scalar operands remain invalid for every scalar cast target", [] {
         const std::string targets[] = {"bool", "int", "unsigned", "float"};
         const std::pair<std::string, std::string> operands[] = {
             {"null", "null"}, {"[1]", "list"}, {"{\"key\": 2}", "dict"},
             {"buffer(1)", "buffer"}, {"int", "type"}, {"\"1\"", "String"}
         };
         for (const auto& target : targets) {
             for (const auto& operand : operands) {
                 expectDiagnostic("start {\n print(" + target + "(" + operand.first + "))\n}",
                                  "cannot cast " + operand.second + " to " + target);
             }
             expectDiagnostic("class Item {}\nstart {\n print(" + target + "(Item()))\n}",
                              "cannot cast Item to " + target);
             expectDiagnostic("start {\n handle value = null\n print(" + target +
                                  "(value))\n}",
                              "cannot cast handle to " + target);
             expectDiagnostic("start {\n list values = [1]\n print(" + target +
                                  "(values[0]))\n}",
                              "cannot cast any to " + target);
         }
     }},
    {"float casts branch through ordered range checks before LLVM conversion", [] {
         auto program = parse(R"(class Casts {
             int signedValue(float value) { return int(value) }
             unsigned unsignedValue(float value) { return unsigned(value) }
         }
         start {}
         )");
         const auto ir = simp::CodeGenerator("x86_64-unknown-linux-gnu").generate(program);
         const auto requireGuard = [&](const std::string& lower, const std::string& upper,
                                       const std::string& conversion) {
             const std::regex guard(
                 "  (%t[0-9]+) = fcmp oge double (%t[0-9]+), " + lower + "\n"
                 "  (%t[0-9]+) = fcmp olt double \\2, " + upper + "\n"
                 "  (%t[0-9]+) = and i1 \\1, \\3\n"
                 "  br i1 \\4, label %(cast\\.valid\\.[0-9]+), label %(cast\\.invalid\\.[0-9]+)\n"
                 "\\6:\n"
                 "  call void @simp_exception_raise\\(ptr @\\.simp\\.overflow\\.message, i64 16,[^\n]+\\)\n"
                 "  unreachable\n"
                 "\\5:\n"
                 "  %t[0-9]+ = " + conversion + " double \\2 to i64\n");
             require(std::regex_search(ir, guard),
                     conversion + " must only execute in the range-checked valid block");
         };
         requireGuard("-9223372036854775808\\.0", "9223372036854775808\\.0", "fptosi");
         requireGuard("0\\.0", "18446744073709551616\\.0", "fptoui");
     }},
    {"signed to unsigned casts retain i64 bits without a range check", [] {
         auto program = parse(R"(class Casts {
             unsigned bits(int value) { return unsigned(value) }
         }
         start {}
         )");
         const auto ir = simp::CodeGenerator("x86_64-unknown-linux-gnu").generate(program);
         require(ir.find(" = icmp sge i64 ") == std::string::npos,
                 "signed to unsigned conversion must accept negative values");
         require(ir.find("cast.invalid.") == std::string::npos,
                 "signed to unsigned conversion must not have an overflow branch");
         require(std::regex_search(ir, std::regex("  ret i64 %t[0-9]+\n")),
                 "signed to unsigned conversion must return the i64 payload");
     }},
    {"boolean float conversion uses unordered not-equal without fast math", [] {
         auto program = parse(R"(class Casts {
             bool truth(float value) { return bool(value) }
         }
         start {}
         )");
         const auto ir = simp::CodeGenerator("x86_64-unknown-linux-gnu").generate(program);
         require(std::regex_search(ir, std::regex(" = fcmp une double %t[0-9]+, 0\\.0\n")),
                 "float truth conversion must include NaN and exclude both signed zeros");
     }},
}};
} // namespace
