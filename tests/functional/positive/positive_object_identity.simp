class Base {}

class Child : Base {}

start {
    Base first = Base()
    Base same = first
    Base second = Base()
    print(first == same)
    print(first != same)
    print(first == second)
    print(first != second)

    Child derived = Child()
    Base baseView = derived
    Base sameBaseView = derived
    print(derived == baseView)
    print(derived != baseView)
    print(baseView == sameBaseView)

    Base nullBase = null
    Child nullChild = null
    print(nullBase == null)
    print(nullBase != null)
    print(baseView == null)
    print(baseView != null)
    print(nullBase == nullChild)
    print(nullBase != nullChild)
}
