"""Compile/link/run bound callbacks and the public native bridge, hermetically."""

import argparse
from pathlib import Path
import signal
import subprocess
import tempfile

from program_runner import environment, invoke


NATIVE = """
class Native {
    handle register(callback<int(int)> cb)
    int invoke(handle registration, int value)
    void release(handle registration)
    void dispose(handle registration)
    void collect()
    void foreign(handle registration)
    void registeredForeign(handle registration)
    void unlocked(handle registration)
    handle transfer(callback<int(int)> cb)
    handle accept(handle transfer)
    void cancel(handle transfer)
    void transferWorker(handle transfer)
    void unregisteredEnter()
}
handle Native.register(callback<int(int)> cb) from "fixture_callback_register"
int Native.invoke(handle registration, int value) from "fixture_callback_invoke"
void Native.release(handle registration) from "fixture_callback_release"
void Native.dispose(handle registration) from "fixture_callback_dispose"
void Native.collect() from "fixture_callback_collect"
void Native.foreign(handle registration) from "fixture_callback_foreign"
void Native.registeredForeign(handle registration) from "fixture_callback_registered_foreign"
void Native.unlocked(handle registration) from "fixture_callback_unlocked"
handle Native.transfer(callback<int(int)> cb) from "fixture_callback_transfer"
handle Native.accept(handle transfer) from "fixture_callback_accept"
void Native.cancel(handle transfer) from "fixture_callback_cancel"
void Native.transferWorker(handle transfer) from "fixture_callback_transfer_worker"
void Native.unregisteredEnter() from "fixture_callback_unregistered_enter"
"""


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--compiler", type=Path, required=True)
    parser.add_argument("--clang", required=True)
    parser.add_argument("--include", type=Path, required=True)
    parser.add_argument("--fixture", type=Path, required=True)
    parser.add_argument("--work", type=Path, required=True)
    parser.add_argument("--sanitize", default="")
    args = parser.parse_args()
    args.work.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(dir=args.work, prefix="callbacks-") as directory:
        work = Path(directory)
        env = environment(work)
        native = work / "native.o"
        flags = [f"-fsanitize={args.sanitize}"] if args.sanitize else []
        result = invoke([args.clang, "-std=c11", "-I", str(args.include.resolve()),
                         "-c", str(args.fixture.resolve()), "-o", str(native)] + flags, work, env)
        assert result.returncode == 0, result.stderr
        result = invoke([args.clang, "-x", "c++", "-std=c++17", "-fsyntax-only",
                         "-I", str(args.include.resolve()), "-"], work, env,
                        '#include <simp/Callbacks.h>\n'
                        'static_assert(sizeof(SimpCallbackArgument) >= sizeof(void *));\n')
        assert result.returncode == 0, result.stderr
        cases = [
            ("transfer_roots", NATIVE + """
class Payload {
    int run(int n) { return n + 2 }
    destroy { print("disposed") }
}
class Factory {
    handle prepare(Native n) { return n.transfer(Payload().run) }
}
start {
    Native n = Native()
    handle transfer = Factory().prepare(n)
    n.collect()
    handle context = n.accept(transfer)
    n.collect()
    print(n.invoke(context, 40))
    n.release(context)
    n.dispose(context)
    n.collect()
    transfer = Factory().prepare(n)
    n.collect()
    n.cancel(transfer)
    n.collect()
    transfer = Factory().prepare(n)
    n.transferWorker(transfer)
    n.collect()
}
""", "42disposeddisposeddisposed", None),
            ("managed_reentry", """
start {
    inline() {
        if (simp_runtime_managed_enter() != 0) abort();
        simp_runtime_managed_leave(0);
        simp_runtime_gil_release();
        int acquired = simp_runtime_managed_enter();
        if (acquired != 1 || simp_runtime_managed_enter() != 0) abort();
        simp_gc_collect();
        simp_runtime_managed_leave(0);
        simp_runtime_managed_leave(acquired);
        simp_runtime_gil_acquire();
    }
    print("reentered")
}
""", "reentered", None),
            ("unregistered_reentry", NATIVE + """
start { Native().unregisteredEnter() }
""", "", "native reentry requires a registered thread"),
            ("transfer_signature", """
class Payload { void run() {} }
start {
    callback<void()> cb = Payload().run
    inline(callback<void()> cb) {
        simp_callback_transfer_prepare(*cb, "callback<int(int)>");
    }
}
""", "", "signature mismatch"),
            ("native_abis", NATIVE + """
class W {
    strg mixed(strg text, int i, unsigned u, float f, bool b,
               list xs, dict d, buffer bytes, handle opaque) {
        Native().collect()
        print("{} {} {} {} {} {} {} {}", i, u, f, b,
              xs.length, d.length, bytes.length, opaque == null)
        return text
    }
    bool yes(bool b) { return not b }
    unsigned twice(unsigned n) { return n + n }
    float half(float n) { return n / 2.0 }
    void ping() { print("ping") }
    list keepList(list values) { Native().collect()
        return values }
    callback<int(int)> echo(callback<int(int)> cb) { return cb }
    int add(int n) { return n + 2 }
}
start {
    W w = W()
    callback<strg(strg,int,unsigned,float,bool,list,dict,buffer,handle)> mixed = w.mixed
    callback<bool(bool)> yes = w.yes
    callback<unsigned(unsigned)> twice = w.twice
    callback<float(float)> half = w.half
    callback<void()> ping = w.ping
    callback<list(list)> keep = w.keepList
    list kept = null
    callback<callback<int(int)>(callback<int(int)>)> echo = w.echo
    callback<int(int)> add = w.add
    callback<int(int)> echoed = null
    strg text = "native"
    strg output = null
    list xs = [1, 2]
    dict d = {"key": 3}
    buffer bytes = buffer(3)
    inline(callback<strg(strg,int,unsigned,float,bool,list,dict,buffer,handle)> mixed,
           callback<bool(bool)> yes, callback<unsigned(unsigned)> twice,
           callback<float(float)> half, callback<void()> ping,
           callback<list(list)> keep, list kept,
           callback<callback<int(int)>(callback<int(int)>)> echo,
           callback<int(int)> add, callback<int(int)> echoed,
           strg text, strg output, list xs, dict d, buffer bytes) {
        SimpCallbackContext *c = simp_callback_acquire(*mixed,
            "callback<String(String,int,unsigned,float,bool,list,dict,buffer,handle)>");
        typedef void *(*Mixed)(SimpCallbackContext *, void *, int64_t, uint64_t,
                              double, _Bool, void *, void *, void *, void *);
        *output = ((Mixed)simp_callback_adapter(c))(c, *text, -4,
            UINT64_C(18446744073709551615), 1.5, 1, *xs, *d, *bytes, NULL);
        simp_callback_release(c);
        simp_callback_dispose(c);
        c = simp_callback_acquire(*yes, "callback<bool(bool)>");
        if (((_Bool (*)(SimpCallbackContext *, _Bool))simp_callback_adapter(c))(c, 1))
            abort();
        simp_callback_release(c);
        simp_callback_dispose(c);
        c = simp_callback_acquire(*twice, "callback<unsigned(unsigned)>");
        if (((uint64_t (*)(SimpCallbackContext *, uint64_t))simp_callback_adapter(c))(c, 10) != 20)
            abort();
        simp_callback_release(c);
        simp_callback_dispose(c);
        c = simp_callback_acquire(*half, "callback<float(float)>");
        if (((double (*)(SimpCallbackContext *, double))simp_callback_adapter(c))(c, 7.0) != 3.5)
            abort();
        simp_callback_release(c);
        simp_callback_dispose(c);
        c = simp_callback_acquire(*ping, "callback<void()>");
        ((void (*)(SimpCallbackContext *))simp_callback_adapter(c))(c);
        simp_callback_release(c);
        simp_callback_dispose(c);
        c = simp_callback_acquire(*keep, "callback<list(list)>");
        void *only_argument = simp_gc_alloc_array(5);
        *kept = ((void *(*)(SimpCallbackContext *, void *))simp_callback_adapter(c))(c, only_argument);
        simp_callback_release(c);
        simp_callback_dispose(c);
        c = simp_callback_acquire(*echo, "callback<callback<int(int)>(callback<int(int)>)>");
        SimpCallbackArgument arguments[1] = {{0}}, result = {0};
        arguments[0].pointer = *add;
        simp_callback_context_invoke(c, "callback<callback<int(int)>(callback<int(int)>)>",
                                     arguments, &result);
        *echoed = result.pointer;
        simp_callback_release(c);
        simp_callback_dispose(c);
    }
    print(output)
    print(echoed(40))
    print(kept.length)
}
""", "-4 18446744073709551615 1.5 true 2 1 3 truepingnative425", None),
            ("contexts", """
class W {
    int run(int n) { return n + 1 }
    float run(float n) { return n + 0.5 }
}
class Use {
    int use(callback<int(int)> cb, int value) { return cb(value) }
    float use(callback<float(float)> cb, float value) { return cb(value) }
    callback<int(int)> saved
    Use(callback<int(int)> cb) { saved = cb }
    Use(callback<float(float)> cb, float value) { print(cb(value)) }
    callback<int(int)> get()
}
callback<int(int)> Use.get() { return saved }
start {
    W w = W()
    Use u = Use(w.run)
    print(u.use(w.run, 41))
    print(u.use(w.run, 1.5))
    print(u.get()(9))
}
""", "42210", None),
            ("simple_roots", NATIVE + """
class W {
    int run(int n) { return n + 40 }
    destroy { print("finalized") }
}
class Box {
    callback<int(int)> cb
    callback<int(int)> make() {
        W local = W()
        return local.run
    }
}
start {
    Native n = Native()
    Box b = Box()
    print(b.cb == null)
    b.cb = b.make()
    n.collect()
    print(b.cb(2))
}
""", "true42", None),
            ("values", """
class Worker {
    int bias
    Worker(int initial) { bias = initial }
    int add(int n) { return bias + n }
    float add(float n) { return n + 0.5 }
    strg echo(strg value) { return value }
    Worker object(Worker value) { return value }
    void ping() { print("ping") }
}
class Box {
    callback<int(int)> value
    callback<int(int)> pass(callback<int(int)> cb) { return cb }
    int apply(callback<int(int)> cb, int x) { return cb(x) }
}
class Factory {
    int count
    Worker make() {
        count += 1
        return Worker(10)
    }
}
start {
    Worker w(20)
    Box b
    b = Box()
    b.value = w.add
    callback<int(int)> cb = b.pass(b.value)
    print("{} {} {}", cb(2), b.value(3), b.apply(w.add, 4))
    callback<float(float)> fp = w.add
    print(fp(1.5))
    callback<strg(strg)> text = w.echo
    print(text("managed"))
    callback<Worker(Worker)> obj = w.object
    Worker same = obj(w)
    print(same.bias)
    callback<void()> ping = w.ping
    ping()
    Factory f = Factory()
    callback<int(int)> once = f.make().add
    print("{} {}", f.count, once(7))
    callback<int(int)> empty = null
    print("{} {} {}", empty == null, cb == cb, cb != empty)
    callback<int(int)> independent = w.add
    print(cb != independent)
    print(type(empty))
    print(type(cb))
}
""", "22 23 242managed20ping1 17true true truetruenullcallback<int(int)>", None),
            ("exceptions", """
class Worker {
    int fail(int n) { return 1 / n }
    destroy { print("destroyed") }
}
start {
    Worker w = Worker()
    callback<int(int)> cb = w.fail
    try { print(cb(0)) } except(Exception) as e { print("caught") }
    w.destroy()
    try { print(cb(1)) } except(Exception) as e { print("dead") }
    try {
        callback<int(int)> invalid = w.fail
    } except(Exception) as e { print("capture dead") }
    Worker nil = null
    try {
        callback<int(int)> invalid = nil.fail
    } except(Exception) as e { print("capture null") }
    callback<int(int)> empty = null
    try { print(empty(1)) } except(Exception) as e { print("empty") }
}
""", "caughtdestroyeddeadcapture deadcapture nullempty", None),
            ("virtual", """
class First { int f() { return 1 } }
class Base { int value(int n) { return n + 2 } }
class Derived : First, Base { int value(int n) { return n + 20 } }
class Left : virtual Base {}
class Right : virtual Base {}
class Diamond : Left, Right { int value(int n) { return n + 200 } }
start {
    Derived d = Derived()
    Base b = d
    callback<int(int)> secondary = b.value
    callback<int(int)> qualified = d.Base.value
    print("{} {}", secondary(1), qualified(2))
    Diamond diamond = Diamond()
    Base virtualBase = diamond
    callback<int(int)> virtualCb = virtualBase.value
    callback<int(int)> virtualQualified = diamond.Left.Base.value
    print("{} {}", virtualCb(1), virtualQualified(2))
}
""", "21 22201 202", None),
            ("native_roots", NATIVE + """
class Worker {
    int base
    Worker(int initial) { base = initial }
    int add(int value) { return base + value }
    destroy { print("finalized") }
}
class Factory {
    handle make(Native native) {
        Worker w(40)
        return native.register(w.add)
    }
    handle quiet(Native native) {
        Quiet w = Quiet()
        return native.register(w.add)
    }
}
class Quiet { int add(int value) { return value } }
start {
    Native n = Native()
    Factory factory = Factory()
    handle registration = factory.make(n)
    n.collect()
    print(n.invoke(registration, 2))
    n.release(registration)
    n.collect()
    n.dispose(registration)
    int count = 0
    while (count < 50) {
        handle next = factory.quiet(n)
        n.invoke(next, count)
        n.release(next)
        n.dispose(next)
        count += 1
    }
    print(count)
}
""", "42finalized50", None),
            ("native_exception", NATIVE + """
class Worker { int fail(int n) { return 1 / n } }
start {
    Native n = Native()
    Worker w = Worker()
    handle registration = n.register(w.fail)
    try { print(n.invoke(registration, 0)) } except(Exception) as e { print("escaped") }
}
""", "", "uncaught Simple exception"),
            ("released", NATIVE + """
class Worker { int run(int n) { return n } }
start {
    Native n = Native()
    Worker w = Worker()
    handle registration = n.register(w.run)
    n.release(registration)
    print(n.invoke(registration, 1))
}
""", "", "registration has been released"),
            ("foreign", NATIVE + """
class Worker { int run(int n) { return n } }
start {
    Native n = Native()
    Worker w = Worker()
    handle registration = n.register(w.run)
    n.foreign(registration)
}
""", "", "registered Simple thread"),
            ("reentrant", NATIVE + """
class Worker {
    Native native
    handle registration
    int depth
    int run(int n) {
        if (depth == 0) {
            depth = 1
            int inner = native.invoke(registration, n + 1)
            depth = 0
            return inner + 10
        }
        native.collect()
        return n
    }
}
start {
    Native n = Native()
    Worker w = Worker()
    w.native = n
    w.registration = n.register(w.run)
    print(n.invoke(w.registration, 1))
    n.release(w.registration)
    n.dispose(w.registration)
}
""", "12", None),
            ("in_flight_release", NATIVE + """
class Worker {
    Native native
    handle registration
    int run(int n) { native.release(registration)
        return n }
}
start {
    Native n = Native()
    Worker w = Worker()
    w.native = n
    w.registration = n.register(w.run)
    n.invoke(w.registration, 1)
}
""", "", "in-flight registration"),
            ("native_signature", """
class W { int run(int n) { return n } }
start {
    W w = W()
    callback<int(int)> cb = w.run
    inline(callback<int(int)> cb) {
        simp_callback_acquire(*cb, "callback<float(float)>");
    }
}
""", "", "signature mismatch"),
            ("native_destroyed", NATIVE + """
class W {
    int run(int n) { return n }
    destroy {}
}
start {
    Native n = Native()
    W w = W()
    handle registration = n.register(w.run)
    w.destroy()
    n.invoke(registration, 1)
}
""", "", "object has been destroyed"),
            ("native_caught", NATIVE + """
class W {
    int run(int n) {
        try { return 1 / n } except() { return -1 }
    }
}
start {
    Native n = Native()
    W w = W()
    handle registration = n.register(w.run)
    print(n.invoke(registration, 0))
    n.release(registration)
    n.dispose(registration)
}
""", "-1", None),
            ("registered_foreign", NATIVE + """
class W { int run(int n) { return n } }
start {
    Native n = Native()
    W w = W()
    handle registration = n.register(w.run)
    n.registeredForeign(registration)
}
""", "", "foreign-thread invocation"),
            ("unlocked", NATIVE + """
class W { int run(int n) { return n } }
start {
    Native n = Native()
    W w = W()
    handle registration = n.register(w.run)
    n.unlocked(registration)
}
""", "", "holding the runtime lock"),
            ("undisposed", NATIVE + """
class W { int run(int n) { return n } }
start {
    Native n = Native()
    W w = W()
    handle registration = n.register(w.run)
    n.release(registration)
}
""", "", "undisposed callback registrations"),
            ("double_release", NATIVE + """
class W { int run(int n) { return n } }
start {
    Native n = Native()
    W w = W()
    handle registration = n.register(w.run)
    n.release(registration)
    n.release(registration)
}
""", "", "registration has been released"),
            ("early_dispose", NATIVE + """
class W { int run(int n) { return n } }
start {
    Native n = Native()
    W w = W()
    handle registration = n.register(w.run)
    n.dispose(registration)
}
""", "", "idle released owner-thread registration"),
            ("argument_destroys", """
class W {
    int run(int n) { return 42 }
    destroy {}
}
class Killer {
    int kill(W object) { object.destroy()
        return 1 }
}
start {
    W w = W()
    callback<int(int)> cb = w.run
    try { print(cb(Killer().kill(w))) } except() { print("dead") }
}
""", "dead", None),
        ]
        for name, source, output, diagnostic in cases:
            path = work / f"{name}.simp"
            path.write_text(source)
            executable = work / name
            result = invoke([str(args.compiler.resolve()), str(path), str(native),
                             "-o", str(executable)] + (["-g"] if name == "values" else []),
                            work, env)
            assert result.returncode == 0, f"{name}: {result.stderr}"
            result = invoke([str(executable)], work, env)
            if diagnostic:
                assert result.returncode == -signal.SIGABRT and diagnostic in result.stderr, (name, result)
                assert "escaped" not in result.stdout, result
            else:
                assert result.returncode == 0 and result.stdout == output, (name, result)
                assert not result.stderr, (name, result)
            print(f"PASS {name}")

        prefix = """class W {
    int run(int n) { return n }
    float run(float n) { return n }
    private:
    int secret(int n) { return n }
    destroy {}
}
"""
        negative = [
            ("mismatch", "callback<float(int)> cb = w.run", "does not match callback signature"),
            ("arity", "callback<int(int)> cb = w.run\n cb()", "argument count mismatch"),
            ("argument", 'callback<int(int)> cb = w.run\n cb("x")', "argument type mismatch"),
            ("access", "callback<int(int)> cb = w.secret", "not accessible"),
            ("ambiguity", "print(w.run)", "ambiguous callback capture"),
            ("collection", "callback<int(int)> cb = w.run\n list values = [cb]", "list elements"),
            ("type_test", "callback<int(int)> cb = w.run\n print(cb is int)", "does not support operand"),
            ("conversion", "callback<int(int)> cb = w.run\n callback<float(float)> other = cb", "cannot initialize"),
            ("void_argument", "callback<int(void)> cb = w.run", "expected type name"),
            ("descriptor", "callback<type()> cb = w.run", "type descriptors"),
            ("free", "callback<int(int)> cb = W.run", "undefined variable"),
            ("cast", "callback<int(int)> cb = w.run\n handle h = cb as handle", "does not support source"),
            ("callback_test", "callback<int(int)> cb = w.run\n print(cb is callback<int(int)>)",
             "callback type tests and casts are not supported"),
            ("default", "callback<int(int)> cb\n cb(1)", "may be uninitialized"),
            ("overload_context", """Use u = Use()
 u.use(w.run)
""", "ambiguous between overloads"),
            ("destructor", "callback<void()> cb = w.destroy", "destructors cannot be captured"),
            ("map", 'callback<int(int)> cb = w.run\n dict d = {"cb": cb}', "dict values"),
            ("inheritance_ambiguity", "Both b = Both()\n callback<int(int)> cb = b.run",
             "ambiguous inherited callback method"),
            ("private_base", "Hidden h = Hidden()\n callback<int(int)> cb = h.A.run",
             "not accessible"),
        ]
        prefix += """class Use {
    int use(callback<int(int)> cb) { return cb(1) }
    float use(callback<float(float)> cb) { return cb(1.0) }
}
class A { int run(int n) { return n } }
class B { int run(int n) { return n } }
class Both : A, B {}
class Hidden : private A {}
"""
        for name, statement, diagnostic in negative:
            path = work / f"negative_{name}.simp"
            path.write_text(prefix + "start {\n W w = W()\n" + statement + "\n}\n")
            result = invoke([str(args.compiler.resolve()), str(path), "--check-only"], work, env)
            assert result.returncode != 0 and diagnostic in result.stderr, (name, result)
            print(f"PASS negative_{name}")
        module = work / "modules" / "callbacks" / "1.0.0"
        module.mkdir(parents=True)
        (module / "simp-package.toml").write_text(
            '[package]\nname = "callbacks"\nversion = "1.0.0"\n'
            'source = "model.simp"\nexport = "namespace:Model"\n')
        (module / "model.simp").write_text("""
namespace Model {
    class Cell {
        int n
        Cell(int initial) { n = initial }
        Cell echo(Cell other) { return other }
        callback<Cell(Cell)> capture() { return echoReceiver().echo }
        Cell echoReceiver() { return Cell(n) }
    }
}
""")
        path = work / "imported.simp"
        path.write_text("""
import callbacks as Cb
start {
    Cb.Cell cell(42)
    callback<Cb.Cell(Cb.Cell)> cb = cell.capture()
    Cb.Cell result = cb(cell)
    print(result.n)
    inline(callback<Cb.Cell(Cb.Cell)> cb, Cb.Cell cell, Cb.Cell result) {
        SimpCallbackContext *context = simp_callback_acquire(*cb, "callback<Model.Cell(Model.Cell)>");
        *result = ((void *(*)(SimpCallbackContext *, void *))simp_callback_adapter(context))(context, *cell);
        simp_callback_release(context);
        simp_callback_dispose(context);
    }
    print(result.n)
}
""")
        executable = work / "imported"
        result = invoke([str(args.compiler.resolve()), str(path), "-o", str(executable)],
                        work, env)
        assert result.returncode == 0, result.stderr
        result = invoke([str(executable)], work, env)
        assert result.returncode == 0 and result.stdout == "4242" and not result.stderr, result
        print("PASS imported_namespace")
    print(f"Checked {len(cases) + len(negative) + 1} callback cases")


if __name__ == "__main__":
    main()
