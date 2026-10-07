"""Run two consumers on the isolated bus supplied by the GTK fixture."""

import select
import subprocess
import sys


primary = subprocess.Popen([sys.argv[1]], stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                           text=True)
try:
    assert select.select([primary.stdout], [], [], 10)[0], "primary never activated"
    assert primary.stdout.readline() == "first\n", "wrong primary activation"
    secondary = subprocess.run([sys.argv[1]], capture_output=True, text=True, timeout=15)
    output, error = primary.communicate(timeout=15)
    assert primary.returncode == 0 and output == "second\n" and not error, (
        primary.returncode, output, error)
    assert secondary.returncode == 0 and not secondary.stdout and not secondary.stderr, (
        secondary.returncode, secondary.stdout, secondary.stderr)
    print("single-instance")
finally:
    if primary.poll() is None:
        primary.terminate()
        primary.wait(timeout=10)
