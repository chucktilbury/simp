#pragma once

#include "cwhip/Ast.hpp"

#include <functional>
#include <initializer_list>
#include <string>
#include <utility>
#include <vector>

namespace cwhip_test {

using Test = std::pair<std::string, std::function<void()>>;

struct TestGroupRegistration {
    TestGroupRegistration(int order, std::initializer_list<Test> tests);
};

std::vector<Test> registeredTests();
void require(bool condition, const std::string& message);
cwhip::Program parse(const std::string& source, const std::string& name = "test.cw");
void expectDiagnostic(const std::string& source, const std::string& expected);
void expectValid(const std::string& source);

} // namespace cwhip_test
