# Fibonacci sequence in Simple

namespace Math {
    class Fib {

        # Naive recursive version (fine for small n)
        int compute (int n) {
            if (n <= 1) {
                return (n)
            }
            int v = compute(n - 1) + compute(n - 2)
            ; print(format("{val}, ", val=v))
            return(v)
        }
    }
}

start {
    Math.Fib fib = Math.Fib()
    int n = 0
    print("the first 10 fibonacci numbers")
    while(n <= 10) {
        print(fib.compute(n))
        n+=1
    }
}
