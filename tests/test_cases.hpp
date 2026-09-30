#pragma once

#include "simp/Ast.hpp"

#include <functional>
#include <initializer_list>
#include <string>
#include <utility>
#include <vector>

namespace simp_test {

using Test = std::pair<std::string, std::function<void()>>;

struct TestGroupRegistration {
    TestGroupRegistration(int order, std::initializer_list<Test> tests);
};

std::vector<Test> registeredTests();
void require(bool condition, const std::string& message);
simp::Program parse(const std::string& source, const std::string& name = "test.simp");
void expectDiagnostic(const std::string& source, const std::string& expected);
void expectValid(const std::string& source);

} // namespace simp_test
