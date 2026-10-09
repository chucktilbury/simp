# Defines and uses a class inside one namespace with a qualified constructor call.
namespace Orchard {
    class Apple {
        Apple() {}
        int sweetness() {
            return 42
        }
    }
}

start {
    Orchard.Apple apple = Orchard.Apple()
    print(apple.sweetness())
}
