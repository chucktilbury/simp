class Foo {
 Foo() { raise(Exception("this is the string")) }
}
class Bar : Foo {
 Bar() {
  try { super Foo() }
  except() { print("caught the first time")
             raise() }
 }
}
start {
 try { Bar b() }
 except() as e { print(e)
                print("caught the second time") }
}
