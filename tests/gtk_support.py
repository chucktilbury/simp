"""Explicit optional-GTK fixtures; no dependency on an existing desktop."""

from contextlib import contextmanager
import os
from pathlib import Path
import select
import subprocess


@contextmanager
def headless(xvfb: str, work: Path, env: dict[str, str]):
    assert xvfb, "Configured GTK programs require --xvfb"
    env = dict(env)
    env.update({"GTK_A11Y": "none", "GSK_RENDERER": "cairo",
                "GDK_DISABLE": "gl"})
    env.pop("DBUS_SESSION_BUS_ADDRESS", None)
    read_fd, write_fd = os.pipe()
    with (work / "xvfb.log").open("w") as log:
        server = subprocess.Popen(
            [xvfb, "-displayfd", str(write_fd), "-screen", "0", "1024x768x24",
             "-nolisten", "tcp"], pass_fds=(write_fd,), stdout=log, stderr=log, env=env)
        os.close(write_fd)
        try:
            assert select.select([read_fd], [], [], 10)[0], "Xvfb failed to start"
            display = os.read(read_fd, 64).decode().strip()
            assert display.isdigit() and server.poll() is None, (work / "xvfb.log").read_text()
            env["DISPLAY"] = ":" + display
            with (work / "dbus.log").open("w") as bus_log:
                bus = subprocess.Popen(
                    ["dbus-daemon", "--session", "--nofork", "--nopidfile",
                     "--print-address=1"], stdout=subprocess.PIPE, stderr=bus_log,
                    text=True, env=env)
                try:
                    assert select.select([bus.stdout], [], [], 10)[0], "session bus failed to start"
                    address = bus.stdout.readline().strip()
                    assert address.startswith("unix:") and bus.poll() is None, (
                        work / "dbus.log").read_text()
                    env["DBUS_SESSION_BUS_ADDRESS"] = address
                    yield env
                finally:
                    bus.terminate()
                    bus.wait(timeout=10)
                    bus.stdout.close()
        finally:
            os.close(read_fd)
            server.terminate()
            server.wait(timeout=10)
