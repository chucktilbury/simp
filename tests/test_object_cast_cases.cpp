#include "test_cases.hpp"

#include "simp/CodeGenerator.hpp"

namespace {
using namespace simp_test;

const TestGroupRegistration registration{2, {
    {"parenthesized cast receivers support standalone void method calls", [] {
         expectValid(R"(class Bar {
             int val
             Bar(int v) { val = v }
             void show() { print(format("class Bar {}", val)) }
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
    {"checked casts support collection payload types and aliases", [] {
         expectValid(R"(class Foo {}
             start {
                 list values = [1, 2u, true, 3.5, "text", [4], {"x": 5},
                                buffer(1), type(int), Foo(), null]
                 int i = values[0] as int
                 unsigned u = values[1] as unsigned
                 bool b = values[2] as bool
                 float f = values[3] as float
                 strg s = values[4] as strg
                 list l = values[5] as list
                 dict d = values[6] as dict
                 buffer bytes = values[7] as buffer
                 type descriptor = values[8] as type
                 Foo object = values[9] as Foo
                 print(i)
                 print(u)
                 print(b)
                 print(f)
                 print(s)
                 print(l.length)
                 print(d.length)
                 print(bytes.length)
                 print(descriptor)
                 print(object is Foo)
                 print(type(values[10] as null))
                 print(values[10] as any)
                 strg alias = values[4] as String
                 print(alias)
             })");
     }},
    {"checked casts reject impossible sources and targets", [] {
         expectDiagnostic("class Foo {}\nstart { print(1 as Foo) }",
                          "checked cast from 'int' to 'Foo' is impossible");
         expectDiagnostic("class Foo {}\nstart { print(Foo() as int) }",
                          "checked cast from 'Foo' to 'int' is impossible");
         expectDiagnostic("class Foo {}\nstart { print(Foo() as void) }",
                          "checked cast target 'void' is not supported");
         expectDiagnostic("class Foo {}\nstart { print(Foo() as Missing) }",
                          "unknown type name 'Missing'");
         expectDiagnostic("class Foo {}\nstart { print(null is null) }",
                          "expected type name after 'is'");
         expectDiagnostic("class Foo { void noValue() {} }\n"
                          "start { print(Foo().noValue() as int) }",
                          "checked cast does not support source type 'void'");
         expectDiagnostic("class Foo {}\nstart { print(Foo() as) }",
                          "expected type name after 'as'");
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
