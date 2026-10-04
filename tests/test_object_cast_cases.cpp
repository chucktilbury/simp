#include "test_cases.hpp"

#include "simp/CodeGenerator.hpp"

namespace {
using namespace simp_test;

const TestGroupRegistration registration{2, {
    {"parenthesized cast receivers support standalone void method calls", [] {
         expectValid(R"(class Bar {
             int val
             Bar(int v) { val = v }
             void show() { print("class Bar {}"(val)) }
         }
         class Foo {
             list flarp
             Foo() {
                 flarp = []
                 int i = 0
                 while (i <= 10) {
                     flarp.append(Bar(i))
                     i += 1
                 }
             }
         }
         start {
             Foo f()
             (f.flarp[1] as Bar).show()
         })");
     }},
    {"parenthesized noncalls remain invalid expression statements", [] {
         for (const auto& expression : {"(1)", "(1 + 2)", "(b)", "(b.val)",
                                        "(b as Bar)", "(values[0] as Bar)", "(Bar())"}) {
             expectDiagnostic(
                 "class Bar { int val }\nstart {\n Bar b()\n list values = [b]\n " +
                     std::string(expression) + "\n}",
                 "only method calls may be used as expression statements");
         }
     }},
    {"parenthesized statement routing preserves assignments and ordinary calls", [] {
         expectValid(R"(class Bar { int val
             void show() {}
             void invoke() { (show()) }
         }
         start {
             Bar b()
             int value = 1
             value += 1
             b.val = value
             (b).val += 1
             (b.val) = 3
             (b.show())
             ((b)).show()
             b.show()
         })");
     }},
    {"checked object casts accept class references, null, and collection values", [] {
         expectValid(R"(class Foo { int read() { return 7 } }
             class Bar : Foo {}
             class Other {}
             start {
                 list values = [Foo(), Bar(), null, 7]
                 dict mapping = {"value": Bar()}
                 Foo exact = values[0] as Foo
                 Foo base = values[1] as Foo
                 Foo empty = null as Foo
                 Foo unrelated = Other() as Foo
                 Foo scalar = values[3] as Foo
                 print((mapping["value"] as Foo).read())
                 print((base as Bar).read())
                 print((Bar() as Foo as Bar).read())
             })");
     }},
    {"checked object casts reject non-object static sources", [] {
         for (const auto& source : {"1", "true", "1.5", "1u", "[1]",
                                    "{\"a\": 1}", "buffer(1)", "type(1)"}) {
             expectDiagnostic("class Foo {}\nstart { print(" + std::string(source) +
                                  " as Foo) }",
                              "checked object cast requires any or a class reference");
         }
         expectDiagnostic("class Foo {}\nstart { handle h = null\n print(h as Foo) }",
                          "checked object cast requires any or a class reference");
     }},
    {"checked object casts require declared class targets", [] {
         expectDiagnostic("class Foo {}\nstart { print(Foo() as Missing) }",
                          "unknown class 'Missing'");
         for (const auto& target : {"int", "unsigned", "float", "bool", "list", "dict",
                                    "any", "type", "void", "handle", "buffer", "strg"}) {
             expectDiagnostic("class Foo {}\nstart { print(Foo() as " +
                                  std::string(target) + ") }",
                              "class name after 'as'");
         }
         expectDiagnostic("class Foo {}\nstart { print(Foo() as) }",
                          "class name after 'as'");
         expectDiagnostic("class Foo {}\nstart { Foo value = Foo(Foo()) }",
                          "constructor");
     }},
    {"checked object casts normalize qualified namespace targets", [] {
         expectValid(R"(namespace model {
             class Foo { int read() { return 7 } }
             class Casts {
                 Foo convert(list values) { return values[0] as Foo }
             }
         }
         start { print(([model.Foo()][0] as model.Foo).read()) })");
     }},
    {"checked object casts have postfix precedence and compose with calls", [] {
         auto program = parse(R"(class Foo { int read() { return 7 } }
             start { print(([Foo()][0] as Foo).read() + 1) })");
         const auto& sum = *program.statements.front().expressions.front();
         require(sum.kind == simp::ExpressionKind::Binary, "addition must be outside cast");
         require(sum.left->kind == simp::ExpressionKind::Call, "cast must compose with call");
         const auto& cast = *sum.left->left->left;
         require(cast.kind == simp::ExpressionKind::ObjectCast, "expected explicit object cast");
         require(cast.left->kind == simp::ExpressionKind::Index, "index must bind before cast");
         const auto ir = simp::CodeGenerator("x86_64-unknown-linux-gnu").generate(program);
         require(ir.find("call ptr @simp_value_cast_class") != std::string::npos,
                 "cast must invoke returning runtime check");
     }},
}};
} // namespace
