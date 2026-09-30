#include "test_cases.hpp"

#include "simp/Diagnostic.hpp"
#include "simp/Lexer.hpp"
#include "simp/Parser.hpp"
#include "simp/SemanticAnalyzer.hpp"

#include <algorithm>
#include <stdexcept>

namespace simp_test {
namespace {

struct Group {
    int order;
    std::vector<Test> tests;
};

std::vector<Group>& groups() {
    static std::vector<Group> entries;
    return entries;
}

} // namespace

TestGroupRegistration::TestGroupRegistration(int order, std::initializer_list<Test> tests) {
    groups().push_back({order, tests});
}

std::vector<Test> registeredTests() {
    auto ordered = groups();
    std::stable_sort(ordered.begin(), ordered.end(),
                     [](const Group& left, const Group& right) {
                         return left.order < right.order;
                     });
    std::vector<Test> tests;
    for (auto& group : ordered) {
        tests.insert(tests.end(), group.tests.begin(), group.tests.end());
    }
    return tests;
}

void require(bool condition, const std::string& message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

simp::Program parse(const std::string& source, const std::string& name) {
    simp::Lexer lexer(source, name);
    auto program = simp::Parser(lexer.tokenize()).parseProgram();
    simp::SemanticAnalyzer analyzer;
    analyzer.analyze(program);
    return program;
}

void expectDiagnostic(const std::string& source, const std::string& expected) {
    try {
        parse(source);
    } catch (const simp::DiagnosticError& error) {
        require(std::string(error.what()).find(expected) != std::string::npos,
                "diagnostic did not contain: " + expected + "; got: " + error.what());
        return;
    }
    throw std::runtime_error("expected diagnostic containing: " + expected);
}

void expectValid(const std::string& source) {
    parse(source);
}

} // namespace simp_test
