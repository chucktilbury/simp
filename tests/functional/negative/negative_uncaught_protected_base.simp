class Foo {
    Foo() { raise(Exception("original base failure")) }
}
class Bar : Foo {
    Bar() {
        try { super Foo() }
        except() {
            print("caught locally")
            raise()
        }
        finally { print("local cleanup") }
    }
}
start { Bar b() }
