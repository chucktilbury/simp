"""Real GTK 4 adapters and worker scheduling through compiled Simple programs."""

import argparse
import os
from pathlib import Path
import shlex
import signal
import subprocess
import tempfile

from program_runner import environment, invoke


NATIVE = """
import gtk
class F {
    handle button()
    handle entry()
    handle window()
    int clicked(handle object, callback<void()> cb)
    int changed(handle object, callback<void()> cb)
    int closeSignal(handle object, callback<bool()> cb)
    bool disconnect(int token)
    void emit(handle object)
    void text(handle object)
    bool updated(handle object)
    bool request(handle object)
    void drop(handle object)
    void collect()
    void owner()
    void destroyed()
    int count()
    void unblock(handle sem)
    void foreign()
    void unlocked(callback<void()> cb)
}
handle F.button() from "fixture_gtk_button"
handle F.entry() from "fixture_gtk_entry"
handle F.window() from "fixture_gtk_window"
int F.clicked(handle object, callback<void()> cb) from "fixture_gtk_clicked"
int F.changed(handle object, callback<void()> cb) from "fixture_gtk_changed"
int F.closeSignal(handle object, callback<bool()> cb) from "fixture_gtk_close"
bool F.disconnect(int token) from "fixture_gtk_disconnect"
void F.emit(handle object) from "fixture_gtk_emit"
void F.text(handle object) from "fixture_gtk_text"
bool F.updated(handle object) from "fixture_gtk_is_updated"
bool F.request(handle object) from "fixture_gtk_request"
void F.drop(handle object) from "fixture_gtk_drop"
void F.collect() from "fixture_gtk_collect"
void F.owner() from "fixture_gtk_owner"
void F.destroyed() from "fixture_gtk_destroyed"
int F.count() from "fixture_gtk_count"
void F.unblock(handle sem) from "fixture_gtk_unblock"
void F.foreign() from "fixture_gtk_foreign"
void F.unlocked(callback<void()> cb) from "fixture_gtk_unlocked_post"
class Thread {
    handle launch()
    void join(handle thread)
}
handle Thread.launch() from "simp_thread_start"
void Thread.join(handle thread) from "simp_thread_join"
class Sem {
    handle create(int n)
    void wait(handle sem)
    void release(handle sem)
}
handle Sem.create(int n) from "simp_semaphore_create"
void Sem.wait(handle sem) from "simp_semaphore_wait"
void Sem.release(handle sem) from "simp_semaphore_release"
"""

CASES = [
    ("signals", """
class Handler {
    handle button
    handle entry
    int clicked
    int changed
    int nesting
    Handler(handle b, handle e) { button = b
        entry = e }
    void click() {
        F().owner()
        F().collect()
        if (nesting == 0) {
            nesting = 1
            F().emit(button)
            print(F().disconnect(clicked))
            F().text(entry)
        }
        print("click")
    }
    void edit() {
        F().collect()
        print(F().updated(entry))
        print(F().disconnect(changed))
    }
    bool closeYes() { F().collect()
        return true }
    bool closeNo() { return false }
    void trigger() {
        F().emit(button)
        F().emit(button)
        Gtk.Application().quit()
    }
}
start {
    Gtk.Application app = Gtk.Application()
    app.initialize()
    handle b = F().button()
    handle e = F().entry()
    handle w = F().window()
    Handler h = Handler(b, e)
    h.clicked = F().clicked(b, h.click)
    h.changed = F().changed(e, h.edit)
    int close = F().closeSignal(w, h.closeYes)
    print(F().request(w))
    print(F().disconnect(close))
    close = F().closeSignal(w, h.closeNo)
    print(F().request(w))
    F().disconnect(close)
    app.post(h.trigger)
    app.run()
    print(F().disconnect(h.clicked))
    F().drop(b)
    F().drop(e)
    F().drop(w)
    app.shutdown()
}
""", "truetruefalseclicktruetruetrueclickfalse", None),
    ("destroy_during_signal", """
class Handler {
    handle button
    int token
    Handler(handle b) { button = b }
    void click() {
        F().drop(button)
        F().collect()
        print("alive")
    }
}
class Setup {
    int install(handle button) {
        Handler handler = Handler(button)
        return F().clicked(button, handler.click)
    }
}
start {
    Gtk.Application().initialize()
    handle button = F().button()
    int token = Setup().install(button)
    F().collect()
    F().emit(button)
    print(F().disconnect(token))
    F().collect()
    Gtk.Application().shutdown()
}
""", "alivefalse", None),
    ("root_and_cancellation", """
class Payload {
    void note()
    destroy { note() }
    void work() { F().owner()
        F().collect()
        print("retained")
        Gtk.Application().quit() }
}
void Payload.note() from "fixture_gtk_destroyed"
class Setup {
    int submit() { return Gtk.Application().post(Payload().work) }
    int connect(handle button) { return F().clicked(button, Payload().work) }
}
start {
    Gtk.Application app = Gtk.Application()
    app.initialize()
    int cancel = Setup().submit()
    F().collect()
    print(F().count())
    print(app.cancel(cancel))
    print(app.cancel(cancel))
    F().collect()
    print(F().count())
    Setup().submit()
    F().collect()
    app.run()
    F().collect()
    print(F().count())
    handle button = F().button()
    Setup().connect(button)
    F().collect()
    print(F().count())
    F().drop(button)
    F().collect()
    print(F().count())
    Setup().submit()
    app.shutdown()
    F().collect()
    print(F().count())
}
""", "0truefalse1retained2234", None),
    ("worker_progress", """
class Payload {
    int value
    Payload(int n) { value = n }
    void done() {
        F().owner()
        F().collect()
        print(value)
        Gtk.Application().quit()
    }
}
class Worker : Thread {
    handle gate
    Worker(handle sem) { gate = sem }
    void submit(int n) { Gtk.Application().post(Payload(n).done) }
    void run() {
        Sem().wait(gate)
        int n = 0
        while (n < 10000) { n = n + 1 }
        submit(n)
        F().collect()
    }
}
start {
    Gtk.Application app = Gtk.Application()
    app.initialize()
    handle gate = Sem().create(0)
    Worker worker = Worker(gate)
    handle thread = worker.launch()
    F().unblock(gate)
    app.run()
    worker.join(thread)
    Sem().release(gate)
    app.shutdown()
}
""", "10000", None),
    ("shutdown_in_handler", """
class Handler {
    handle button
    Handler(handle b) { button = b }
    void click() { Gtk.Application().shutdown()
        F().collect()
        print("shutdown") }
    void pending() { print("must-not-run") }
    void trigger() {
        Gtk.Application().post(Handler(button).pending)
        F().emit(button)
    }
}
start {
    Gtk.Application().initialize()
    handle b = F().button()
    Handler h = Handler(b)
    int token = F().clicked(b, h.click)
    Gtk.Application().post(h.trigger)
    Gtk.Application().post(h.pending)
    Gtk.Application().run()
    print(F().disconnect(token))
    F().drop(b)
    Gtk.Application().shutdown()
}
""", "shutdownfalse", None),
    ("wrong_thread", """
class Worker : Thread {
    void run() { Gtk.Application().quit() }
}
start {
    Gtk.Application().initialize()
    Worker w = Worker()
    handle thread = w.launch()
    w.join(thread)
}
""", "", "operation requires the GUI owner thread"),
    ("foreign_thread", """
start { Gtk.Application().initialize()
    F().foreign() }
""", "", "operation requires the GUI owner thread"),
    ("post_after_shutdown", """
class Handler { void run() {} }
start { Gtk.Application().initialize()
    Gtk.Application().shutdown()
    Gtk.Application().post(Handler().run) }
""", "", "post requires a live application"),
    ("unlocked_post", """
class Handler { void run() {} }
start { Gtk.Application().initialize()
    F().unlocked(Handler().run) }
""", "", "operation requires the managed runtime lock"),
    ("exception_boundary", """
class Handler { void run() { raise(Exception("gtk boundary")) } }
start { Gtk.Application().initialize()
    Gtk.Application().post(Handler().run)
    Gtk.Application().run() }
""", "", "Simple native callback error: uncaught Simple exception: gtk boundary"),
]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--compiler", type=Path, required=True)
    parser.add_argument("--clang", required=True)
    parser.add_argument("--include", type=Path, required=True)
    parser.add_argument("--native", type=Path, required=True)
    parser.add_argument("--fixture", type=Path, required=True)
    parser.add_argument("--work", type=Path, required=True)
    parser.add_argument("--sanitize", default="")
    parser.add_argument("--xvfb", required=True)
    parser.add_argument("--build", type=Path)
    args = parser.parse_args()
    args.work.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(dir=args.work, prefix="gtk-") as directory:
        work = Path(directory).resolve()
        env = environment(work)
        env.update({"GTK_A11Y": "none", "GSK_RENDERER": "cairo"})
        native = work / "fixture.o"
        flags = shlex.split(subprocess.check_output(
            ["pkg-config", "--cflags", "gtk4"], text=True))
        if args.sanitize:
            flags.append(f"-fsanitize={args.sanitize}")
        result = invoke([args.clang, "-std=c11", "-Wall", "-Wextra", "-Werror",
                         "-I", str(args.include.resolve()), "-I", str(args.native.resolve()),
                         "-c", str(args.fixture.resolve()), "-o", str(native)] + flags, work, env)
        assert result.returncode == 0, result.stderr
        # A fresh server selects a free display atomically; no system service
        # or pre-existing desktop/session bus is needed.
        read_fd, write_fd = os.pipe()
        log = (work / "xvfb.log").open("w")
        server = subprocess.Popen([args.xvfb, "-displayfd", str(write_fd),
                                   "-screen", "0", "1024x768x24", "-nolisten", "tcp"],
                                  pass_fds=(write_fd,), stdout=log, stderr=log, env=env)
        os.close(write_fd)
        try:
            import select
            assert select.select([read_fd], [], [], 10)[0], "Xvfb failed to start"
            display = os.read(read_fd, 64).decode().strip()
            assert display.isdigit() and server.poll() is None, (work / "xvfb.log").read_text()
            env["DISPLAY"] = ":" + display
            for name, program, expected, diagnostic in CASES:
                source = work / f"{name}.simp"
                source.write_text(NATIVE + program)
                output = work / name
                result = invoke([str(args.compiler.resolve()), str(source), str(native),
                                 "-o", str(output)], work, env)
                assert result.returncode == 0, f"{name}: {result.stderr}"
                result = invoke([str(output)], work, env)
                assert result.stdout == expected, (
                    f"{name}: exit {result.returncode}, {result.stdout!r}, {result.stderr}")
                if diagnostic:
                    assert result.returncode == -signal.SIGABRT and diagnostic in result.stderr, (
                        name, result.returncode, result.stderr)
                else:
                    assert result.returncode == 0, f"{name}: {result.stderr}"
                    # Session bus warnings are external GTK desktop integration;
                    # all critical/errors remain failures.
                    assert "CRITICAL" not in result.stderr and "ERROR" not in result.stderr, result.stderr
                print(f"PASS {name}")
            # The package's native dependencies must survive separate
            # compilation in the existing object link sidecar.
            source = work / "signals.simp"
            object_file = work / "signals.o"
            result = invoke([str(args.compiler.resolve()), str(source), "-c",
                             "-o", str(object_file)], work, env)
            assert result.returncode == 0, result.stderr
            result = invoke([str(args.compiler.resolve()), str(object_file), str(native),
                             "-o", str(work / "object-consumer")], work, env)
            assert result.returncode == 0, result.stderr
            result = invoke([str(work / "object-consumer")], work, env)
            assert result.returncode == 0 and result.stdout == CASES[0][2], result.stderr
            print("PASS compile-only package link sidecar")
            if args.build:
                prefix = work / "installed"
                result = invoke(["cmake", "--install", str(args.build.resolve()),
                                 "--prefix", str(prefix)], work, env)
                assert result.returncode == 0, result.stderr
                source = work / "installed.simp"
                source.write_text("""import gtk
class Handler {
    void run() { print("installed")
        Gtk.Application().quit() }
}
start {
    Gtk.Application().initialize()
    Gtk.Application().post(Handler().run)
    Gtk.Application().run()
    Gtk.Application().shutdown()
}
""")
                result = invoke([str(prefix / "bin/simp"), str(source), "-o",
                                 str(work / "installed") + "-program"], work, env)
                assert result.returncode == 0, result.stderr
                result = invoke([str(work / "installed") + "-program"], work, env)
                assert result.returncode == 0 and result.stdout == "installed", result.stderr
                print("PASS installed consumer")
                result = invoke([str(prefix / "bin/simp"), str(work / "signals.simp"),
                                 str(native), "-o", str(work / "installed-signals")], work, env)
                assert result.returncode == 0, result.stderr
                result = invoke([str(work / "installed-signals")], work, env)
                assert result.returncode == 0 and result.stdout == CASES[0][2], result.stderr
                print("PASS installed signal consumer")
            source = work / "no-import.simp"
            source.write_text('start { print("no gtk") }\n')
            result = invoke([str(args.compiler.resolve()), str(source), "-o",
                             str(work / "no-import")], work, env)
            assert result.returncode == 0, result.stderr
            for output in (work / "no-import", args.compiler.resolve()):
                result = invoke(["ldd", str(output)], work, env)
                assert result.returncode == 0 and "libgtk" not in result.stdout, result.stdout
            print("PASS no-import and compiler linkage")
        finally:
            os.close(read_fd)
            server.terminate()
            server.wait(timeout=10)
            log.close()
    print(f"Checked {len(CASES)} GTK cases, object/installed consumers and link isolation")


if __name__ == "__main__":
    main()
