#include "test_cases.hpp"

#include <algorithm>

namespace {
using namespace simp_test;

const TestGroupRegistration registration{1, {
    {"inline capture paths preserve the C parameter name", [] {
        auto program = parse(
            "class Base { int value }\n"
            "class Child : Base {\n"
            " void update() {\n"
            "  inline (int Base.value) { *value = 7; }\n"
            "  int value = 1\n"
            "  inline (int .value) { *value += 2; }\n"
            "  inline (int value) { *value = 3; }\n"
            " }\n}\nstart {}");
        const auto child = std::find_if(program.classes.begin(), program.classes.end(),
            [](const auto& owner) { return owner.name == "Child"; });
        require(child != program.classes.end(), "Child class missing");
        const auto& body = child->methods[0].body;
        require(body.size() == 4, "capture statements missing");
        require(body[0].inlineCaptures[0].name == "value" &&
                    body[0].inlineCaptures[0].target->kind == simp::ExpressionKind::Member,
                "qualified field capture AST missing");
        require(body[2].inlineCaptures[0].target->left->kind ==
                    simp::ExpressionKind::ImplicitThis &&
                    body[3].inlineCaptures[0].target->kind == simp::ExpressionKind::Identifier,
                "explicit receiver or local precedence lost");
    }},
    {"inline captures use field access checks and exact types", [] {
        expectDiagnostic(
            "class Base { private: int value }\n"
            "class Child : Base { void update() { inline (int value) {} } }\nstart {}",
            "field 'value' is not accessible");
        expectDiagnostic(
            "class Base { private: int value }\n"
            "class Child : Base { void update() { inline (int Base.value) {} } }\nstart {}",
            "field 'value' is not accessible");
        expectDiagnostic(
            "class A { int value }\nclass B { int value }\n"
            "class C : A, B { void update() { inline (int value) {} } }\nstart {}",
            "ambiguous inherited field 'value'");
        expectDiagnostic(
            "class Box { int value\nvoid update() { inline (float value) {} } }\nstart {}",
            "inline capture type 'float' does not match int");
        expectDiagnostic("start { inline (int .value) {} }",
                         "instance field inline capture requires an instance context");
        expectDiagnostic("start { inline (int value) {} }", "undefined inline capture");
        expectDiagnostic(
            "class Box { int value }\nstart { Box box = Box()\n"
            " inline (int box.value) {} }",
            "instance field inline capture requires an instance context");
        expectDiagnostic(
            "class Box { int value\nvoid update(Box other) { inline (int other.value) {} } }\n"
            "start {}",
            "qualified inline capture must name an instance field");
        expectDiagnostic(
            "class Inner { int value }\n"
            "class Outer { Inner item\nvoid update() { inline (int .item.value) {} } }\n"
            "start {}",
            "inline capture field path must contain only base classes");
        expectDiagnostic(
            "class A { int value }\nclass B : A {\n"
            " void update() { inline (int value, int A.value) {} }\n}\nstart {}",
            "duplicate inline capture 'value'");
        expectDiagnostic("start { inline (int .) {} }", "expected capture name");
        expectDiagnostic(
            "class Box { int value\nvoid update() { inline (int .missing) {} } }\nstart {}",
            "has no field 'missing'");
    }},
    {"inline fields respect private inheritance and parameter shadowing", [] {
        expectDiagnostic(
            "class A { int value }\nclass B : private A {}\n"
            "class C : B { void update() { inline (int B.A.value) {} } }\nstart {}",
            "field 'value' is not accessible");
        expectValid(
            "class A { private: int value }\nclass B : A {\n"
            " void update(float value) { inline (float value) { *value += 1.0; } }\n"
            "}\nstart {}");
        expectValid(
            "namespace N {\nclass Base { int value }\n"
            "class Child : Base { void update() { inline (int N.Base.value) {} } }\n"
            "}\nstart {}");
    }},
}};
}
