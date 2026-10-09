"""Drive Cwhip Editor with real X key events and window-manager close.

Ctrl+W must close only the focused tab, while Ctrl+Q and the window close
button must quit cleanly; closing from inside a shortcut callback previously
aborted with "cannot release an in-flight registration".
"""

import argparse
import ctypes
from pathlib import Path
import subprocess
import tempfile
import time

from gtk_support import headless
from program_runner import environment


class XEventData(ctypes.Structure):
    _fields_ = [("type", ctypes.c_int), ("serial", ctypes.c_ulong),
                ("send_event", ctypes.c_int), ("display", ctypes.c_void_p),
                ("window", ctypes.c_ulong), ("message_type", ctypes.c_ulong),
                ("format", ctypes.c_int), ("data", ctypes.c_long * 5)]


class XEvent(ctypes.Union):
    _fields_ = [("client", XEventData), ("pad", ctypes.c_long * 24)]


class Display:
    def __init__(self, name: str):
        self.x11 = ctypes.CDLL("libX11.so.6")
        self.xtest = ctypes.CDLL("libXtst.so.6")
        x11, xtest = self.x11, self.xtest
        x11.XOpenDisplay.restype = ctypes.c_void_p
        x11.XOpenDisplay.argtypes = [ctypes.c_char_p]
        x11.XCloseDisplay.argtypes = [ctypes.c_void_p]
        x11.XStringToKeysym.restype = ctypes.c_ulong
        x11.XStringToKeysym.argtypes = [ctypes.c_char_p]
        x11.XKeysymToKeycode.argtypes = [ctypes.c_void_p, ctypes.c_ulong]
        x11.XDefaultRootWindow.restype = ctypes.c_ulong
        x11.XDefaultRootWindow.argtypes = [ctypes.c_void_p]
        x11.XQueryPointer.argtypes = [ctypes.c_void_p, ctypes.c_ulong] + [ctypes.c_void_p] * 7
        x11.XSetInputFocus.argtypes = [ctypes.c_void_p, ctypes.c_ulong, ctypes.c_int,
                                       ctypes.c_ulong]
        x11.XInternAtom.restype = ctypes.c_ulong
        x11.XInternAtom.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.c_int]
        x11.XSendEvent.argtypes = [ctypes.c_void_p, ctypes.c_ulong, ctypes.c_int,
                                   ctypes.c_long, ctypes.POINTER(XEvent)]
        x11.XSync.argtypes = [ctypes.c_void_p, ctypes.c_int]
        xtest.XTestFakeKeyEvent.argtypes = [ctypes.c_void_p, ctypes.c_uint, ctypes.c_int,
                                            ctypes.c_ulong]
        xtest.XTestFakeMotionEvent.argtypes = [ctypes.c_void_p, ctypes.c_int, ctypes.c_int,
                                               ctypes.c_int, ctypes.c_ulong]
        xtest.XTestFakeButtonEvent.argtypes = [ctypes.c_void_p, ctypes.c_uint, ctypes.c_int,
                                               ctypes.c_ulong]
        self.display = x11.XOpenDisplay(name.encode())
        assert self.display, f"cannot open X display {name}"
        self.root = x11.XDefaultRootWindow(self.display)

    def close(self) -> None:
        self.x11.XCloseDisplay(self.display)

    def settle(self, delay: float = 0.5) -> None:
        self.x11.XSync(self.display, 0)
        time.sleep(delay)

    def window_at(self, x: int, y: int) -> int:
        self.xtest.XTestFakeMotionEvent(self.display, -1, x, y, 0)
        self.x11.XSync(self.display, 0)
        root, child = ctypes.c_ulong(), ctypes.c_ulong()
        value = ctypes.c_int()
        mask = ctypes.c_uint()
        self.x11.XQueryPointer(self.display, self.root, ctypes.byref(root),
                               ctypes.byref(child), ctypes.byref(value),
                               ctypes.byref(value), ctypes.byref(value),
                               ctypes.byref(value), ctypes.byref(mask))
        return child.value

    def focus_editor(self, x: int = 500, y: int = 400) -> int:
        """Click into the source view; no window manager runs, so focus explicitly."""
        deadline = time.monotonic() + 15
        window = 0
        while not window and time.monotonic() < deadline:
            window = self.window_at(x, y)
            if not window:
                time.sleep(0.1)
        assert window, "Cwhip Editor window never appeared"
        time.sleep(0.5)
        self.x11.XSetInputFocus(self.display, window, 1, 0)
        self.xtest.XTestFakeButtonEvent(self.display, 1, 1, 0)
        self.xtest.XTestFakeButtonEvent(self.display, 1, 0, 0)
        self.settle()
        return window

    def keys(self, combo: str) -> None:
        codes = [self.x11.XKeysymToKeycode(self.display, self.x11.XStringToKeysym(name.encode()))
                 for name in combo.split("+")]
        assert all(codes), combo
        for code in codes:
            self.xtest.XTestFakeKeyEvent(self.display, code, 1, 0)
        for code in reversed(codes):
            self.xtest.XTestFakeKeyEvent(self.display, code, 0, 0)
        self.settle()

    def delete_window(self, window: int) -> None:
        event = XEvent()
        event.client.type = 33  # ClientMessage
        event.client.window = window
        event.client.message_type = self.x11.XInternAtom(self.display, b"WM_PROTOCOLS", 0)
        event.client.format = 32
        event.client.data[0] = self.x11.XInternAtom(self.display, b"WM_DELETE_WINDOW", 0)
        assert self.x11.XSendEvent(self.display, window, 0, 0, ctypes.byref(event))
        self.settle()


def finish(process: subprocess.Popen, expect_running: bool) -> str:
    if expect_running:
        assert process.poll() is None, (
            f"Cwhip Editor exited with {process.returncode}:\n{process.communicate()[0]}")
        process.terminate()
        output = process.communicate(timeout=10)[0]
    else:
        output = process.communicate(timeout=10)[0]
        assert process.returncode == 0, f"Cwhip Editor exited with {process.returncode}:\n{output}"
    for marker in ("callback error", "in-flight", "CRITICAL", "AddressSanitizer",
                   "runtime error:", "LeakSanitizer"):
        assert marker not in output, output
    return output


def scenario(executable: Path, work: Path, env: dict[str, str], name: str, action) -> None:
    case = work / name
    case.mkdir(parents=True, exist_ok=True)
    for file_name, text in (("a.txt", "hello\n"), ("b.txt", "world\n")):
        (case / file_name).write_text(text)
    process = subprocess.Popen([str(executable), "a.txt", "b.txt"], cwd=case, env=env,
                               stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    display = Display(env["DISPLAY"])
    try:
        action(display, process, case)
    finally:
        display.close()
        if process.poll() is None:
            process.kill()
            process.communicate()
    print(f"PASS {name}")


def close_tabs(display: Display, process: subprocess.Popen, case: Path) -> None:
    display.focus_editor()
    display.keys("Control_L+w")  # closes b.txt only
    assert process.poll() is None, "Ctrl+W must not quit Cwhip Editor"
    display.keys("x")
    display.keys("Control_L+s")
    saved_path = case / "a.txt"
    deadline = time.monotonic() + 5
    saved = saved_path.read_text()
    while "x" not in saved and time.monotonic() < deadline:
        time.sleep(0.05)
        saved = saved_path.read_text()
    assert "x" in saved and "hello" in saved, saved
    assert (case / "b.txt").read_text() == "world\n"
    display.keys("Control_L+w")  # closes the last tab; a fresh Untitled replaces it
    display.keys("Control_L+w")  # closes that Untitled; another replaces it
    finish(process, expect_running=True)


def dirty_close(display: Display, process: subprocess.Popen, case: Path) -> None:
    display.focus_editor()
    display.keys("x")
    display.keys("Control_L+w")  # prompts Save/Discard/Cancel instead of closing
    display.keys("Control_L+q")  # a pending prompt blocks quitting
    finish(process, expect_running=True)
    assert (case / "b.txt").read_text() == "world\n"


def quit_shortcut(display: Display, process: subprocess.Popen, case: Path) -> None:
    display.focus_editor()
    display.keys("Control_L+q")
    finish(process, expect_running=False)


def quit_after_close(display: Display, process: subprocess.Popen, case: Path) -> None:
    display.focus_editor()
    display.keys("Control_L+w")
    display.keys("Control_L+q")
    finish(process, expect_running=False)


def window_close(display: Display, process: subprocess.Popen, case: Path) -> None:
    window = display.focus_editor()
    display.delete_window(window)
    finish(process, expect_running=False)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--executable", type=Path, required=True)
    parser.add_argument("--xvfb", required=True)
    parser.add_argument("--work", type=Path)
    args = parser.parse_args()
    executable = args.executable.resolve()
    with tempfile.TemporaryDirectory() as temporary:
        work = (args.work or Path(temporary)).resolve()
        work.mkdir(parents=True, exist_ok=True)
        env = environment(work)
        env.pop("WAYLAND_DISPLAY", None)
        env["GDK_BACKEND"] = "x11"
        with headless(args.xvfb, work, env) as env:
            for name, action in (("close_tabs", close_tabs), ("dirty_close", dirty_close),
                                 ("quit_shortcut", quit_shortcut),
                                 ("quit_after_close", quit_after_close),
                                 ("window_close", window_close)):
                scenario(executable, work, env, name, action)


if __name__ == "__main__":
    main()
