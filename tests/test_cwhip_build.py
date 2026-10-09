"""Build the real editor target and require an unchanged second invocation."""

import argparse
from pathlib import Path
import subprocess


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--build", type=Path, required=True)
    parser.add_argument("--cmake", required=True)
    parser.add_argument("--executable", type=Path, required=True)
    parser.add_argument("--compiler", type=Path, required=True)
    args = parser.parse_args()
    command = [args.cmake, "--build", str(args.build), "--target", "cwhip"]
    subprocess.run(command, check=True, timeout=80)
    outputs = (args.executable, args.compiler)
    before = [path.stat().st_mtime_ns for path in outputs]
    subprocess.run(command, check=True, timeout=80)
    after = [path.stat().st_mtime_ns for path in outputs]
    assert before == after, f"No-op cwhip build rewrote outputs: {before} -> {after}"


if __name__ == "__main__":
    main()
